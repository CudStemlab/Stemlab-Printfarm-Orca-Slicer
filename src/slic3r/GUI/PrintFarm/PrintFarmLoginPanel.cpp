// >>> PRINTFARM
#include "PrintFarmLoginPanel.hpp"

#include <wx/sizer.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>

#include "slic3r/GUI/GUI.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "slic3r/GUI/wxExtensions.hpp"
#include "slic3r/GUI/Widgets/Button.hpp"
#include "slic3r/GUI/Widgets/TextInput.hpp"
#include "slic3r/GUI/Widgets/Label.hpp"
#include "slic3r/GUI/PrintFarm/PrintFarmManager.hpp"
#include "libslic3r/AppConfig.hpp"

namespace Slic3r {
namespace GUI {

// Default backend address offered on first run, before any URL is configured.
static const char* PF_DEFAULT_URL = "http://127.0.0.1:8080";

// 3D-FarmLab theme. The accent is Orca's teal (#009688) so the login reads as part
// of the same application rather than a bolted-on dialog.
static const wxColour kPageTop  (30, 30, 34);
static const wxColour kPageBot  (20, 20, 23);
static const wxColour kCard     (45, 45, 48);
static const wxColour kAccent   (0, 150, 136);   // #009688
static const wxColour kHeading  (244, 244, 244);
static const wxColour kSubtle   (150, 154, 158);
static const wxColour kLabel    (200, 200, 200);
static const wxColour kError    (224, 86, 86);

PrintFarmLoginPanel::PrintFarmLoginPanel(wxWindow*             parent,
                                         std::function<void()> on_success,
                                         std::function<void()> on_quit)
    : wxPanel(parent, wxID_ANY)
    , m_on_success(std::move(on_success))
    , m_on_quit(std::move(on_quit))
{
    auto& mgr = PrintFarmManager::instance();
    const bool configured = mgr.is_enabled();

    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(kPageBot);

    // Subtle vertical gradient behind the card for depth.
    Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.GradientFillLinear(GetClientRect(), kPageTop, kPageBot, wxSOUTH);
    });

    const int hpad = FromDIP(36);

    // The card holds the form so the screen reads as an app login page. It carries
    // the page background colour so its rounded corners blend into the gradient,
    // while the card body itself is painted on top.
    auto* card = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(FromDIP(380), -1));
    card->SetBackgroundColour(kPageBot);
    card->Bind(wxEVT_PAINT, [card](wxPaintEvent&) {
        wxPaintDC dc(card);
        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc) return;
        const wxSize  sz = card->GetClientSize();
        const double  r  = card->FromDIP(10);
        gc->SetBrush(wxBrush(kCard));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRoundedRectangle(0, 0, sz.GetWidth(), sz.GetHeight(), r);
        // Teal brand strip along the top edge of the card.
        gc->SetBrush(wxBrush(kAccent));
        gc->DrawRoundedRectangle(0, 0, sz.GetWidth(), card->FromDIP(4) + r, r);
        gc->DrawRectangle(0, card->FromDIP(4), sz.GetWidth(), r);
    });

    auto* card_sizer = new wxBoxSizer(wxVERTICAL);

    // Brand logo.
    wxBitmap logo;
    try {
        logo = create_scaled_bitmap("OrcaSlicer_192px_transparent", card, 56);
    } catch (...) { /* logo is optional; fall through with an empty bitmap */ }
    if (logo.IsOk()) {
        auto* logo_ctrl = new wxStaticBitmap(card, wxID_ANY, logo);
        logo_ctrl->SetBackgroundColour(kCard);
        card_sizer->Add(logo_ctrl, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, FromDIP(28));
    } else {
        card_sizer->AddSpacer(FromDIP(28));
    }

    auto* heading = new wxStaticText(card, wxID_ANY, "3D FarmLab", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    heading->SetFont(::Label::Head_20);
    heading->SetForegroundColour(kHeading);
    heading->SetBackgroundColour(kCard);
    card_sizer->Add(heading, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, FromDIP(12));

    auto* sub = new wxStaticText(card, wxID_ANY, _L("Sign in to your print farm"),
                                 wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    sub->SetFont(::Label::Body_13);
    sub->SetForegroundColour(kSubtle);
    sub->SetBackgroundColour(kCard);
    card_sizer->Add(sub, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, FromDIP(4));

    // Field factory: a caption label above each themed input.
    auto add_field = [&](const wxString& caption, const wxString& value, const wxString& hint, long style) {
        auto* label = new wxStaticText(card, wxID_ANY, caption);
        label->SetFont(::Label::Body_13);
        label->SetForegroundColour(kLabel);
        label->SetBackgroundColour(kCard);
        card_sizer->Add(label, 0, wxLEFT | wxRIGHT | wxTOP, hpad);

        auto* input = new TextInput(card, value, "", "", wxDefaultPosition,
                                    wxSize(-1, FromDIP(34)), style | wxTE_PROCESS_ENTER);
        if (!hint.empty())
            input->GetTextCtrl()->SetHint(hint);
        card_sizer->Add(input, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, hpad);
        return input;
    };

    card_sizer->AddSpacer(FromDIP(16));
    // The server URL stays editable so the backend can be corrected/re-pointed even
    // after it has been configured, without needing to be signed in to reach Settings.
    m_server_url = add_field(_L("Server URL"), wxString::FromUTF8(configured ? mgr.config().url : PF_DEFAULT_URL),
                             wxString::FromUTF8(PF_DEFAULT_URL), 0);
    m_email      = add_field(_L("Email"), wxEmptyString, _L("you@example.com"), 0);
    m_password   = add_field(_L("Password"), wxEmptyString, _L("Your password"), wxTE_PASSWORD);

    m_error = new wxStaticText(card, wxID_ANY, wxEmptyString);
    m_error->SetFont(::Label::Body_12);
    m_error->SetForegroundColour(kError);
    m_error->SetBackgroundColour(kCard);
    card_sizer->Add(m_error, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, hpad);

    m_sign_in = new Button(card, _L("Sign in"));
    m_sign_in->SetStyle(ButtonStyle::Confirm, ButtonType::Window);
    m_sign_in->SetMinSize(wxSize(-1, FromDIP(38)));
    card_sizer->Add(m_sign_in, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, hpad);

    m_quit = new Button(card, _L("Quit"));
    m_quit->SetStyle(ButtonStyle::Regular, ButtonType::Window);
    m_quit->SetMinSize(wxSize(-1, FromDIP(34)));
    card_sizer->Add(m_quit, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, hpad);

    card_sizer->AddSpacer(FromDIP(28));
    card->SetSizer(card_sizer);

    // Center the card in the panel.
    auto* root = new wxBoxSizer(wxVERTICAL);
    root->AddStretchSpacer(1);
    auto* row = new wxBoxSizer(wxHORIZONTAL);
    row->AddStretchSpacer(1);
    row->Add(card, 0, wxALIGN_CENTER_VERTICAL);
    row->AddStretchSpacer(1);
    root->Add(row, 0, wxEXPAND);
    root->AddStretchSpacer(1);
    SetSizer(root);

    m_sign_in->Bind(wxEVT_BUTTON, &PrintFarmLoginPanel::on_sign_in, this);
    m_quit->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { if (m_on_quit) m_on_quit(); });
    // Enter from any field submits.
    for (TextInput* in : {m_server_url, m_email, m_password})
        in->GetTextCtrl()->Bind(wxEVT_TEXT_ENTER, &PrintFarmLoginPanel::on_sign_in, this);

    m_email->GetTextCtrl()->SetFocus();
}

void PrintFarmLoginPanel::set_error(const wxString& msg)
{
    m_error->SetForegroundColour(kError);
    m_error->SetLabel(msg);
    Layout();
}

void PrintFarmLoginPanel::set_busy(bool busy)
{
    m_sign_in->Enable(!busy);
    m_email->Enable(!busy);
    m_password->Enable(!busy);
    if (m_server_url)
        m_server_url->Enable(!busy);
    if (busy) {
        m_error->SetForegroundColour(kSubtle);
        m_error->SetLabel(_L("Signing in…"));
        Layout();
    }
}

void PrintFarmLoginPanel::on_sign_in(wxCommandEvent& /*evt*/)
{
    const std::string email = into_u8(m_email->GetTextCtrl()->GetValue());
    const std::string pwd   = into_u8(m_password->GetTextCtrl()->GetValue());
    if (email.empty() || pwd.empty()) {
        set_error(_L("Please enter your email and password."));
        return;
    }

    auto& mgr = PrintFarmManager::instance();

    // Persist the server URL before authenticating, so the client targets the right
    // backend and subsequent launches know the address.
    {
        std::string url = into_u8(m_server_url->GetTextCtrl()->GetValue());
        while (!url.empty() && (url.back() == ' ' || url.back() == '/'))
            url.pop_back();
        if (url.empty()) {
            set_error(_L("Please enter the Print Farm server URL."));
            return;
        }
        PrintFarmConfig cfg = mgr.config();
        cfg.url = url;
        mgr.set_config(cfg);
        mgr.save_config(wxGetApp().app_config);
        wxGetApp().app_config->save();
    }

    set_busy(true);
    PfResult res = mgr.login(email, pwd);
    set_busy(false);

    if (res.ok) {
        if (m_on_success)
            m_on_success();
        return;
    }
    set_error(wxString::FromUTF8(res.error.empty() ? "Login failed." : res.error));
    m_password->GetTextCtrl()->SetValue(wxEmptyString);
    m_password->GetTextCtrl()->SetFocus();
}

} // namespace GUI
} // namespace Slic3r
// <<< PRINTFARM
