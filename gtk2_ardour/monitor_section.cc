#include <gdkmm/pixbuf.h>

#include "pbd/compose.h"
#include "pbd/error.h"

#include "gtkmm2ext/bindable_button.h"
#include "gtkmm2ext/tearoff.h"
#include "gtkmm2ext/actions.h"

#include "ardour/dB.h"
#include "ardour/monitor_processor.h"
#include "ardour/route.h"
#include "ardour/utils.h"

#include "ardour_ui.h"
#include "monitor_section.h"
#include "public_editor.h"
#include "utils.h"
#include "volume_controller.h"

#include "i18n.h"

using namespace ARDOUR;
using namespace Gtk;
using namespace Gtkmm2ext;
using namespace PBD;
using namespace std;

Glib::RefPtr<ActionGroup> MonitorSection::monitor_actions;
Glib::RefPtr<Gdk::Pixbuf> MonitorSection::big_knob_pixbuf;
Glib::RefPtr<Gdk::Pixbuf> MonitorSection::little_knob_pixbuf;

static bool
fixup_prelight (GdkEventCrossing* /* ignored */, GtkWidget* widget)
{
        GtkRcStyle* style = gtk_rc_style_copy (gtk_widget_get_modifier_style (widget));
        int current = gtk_widget_get_state (widget);

        style->fg[GTK_STATE_PRELIGHT] = style->fg[current];
        style->bg[GTK_STATE_PRELIGHT] = style->bg[current];

        gtk_widget_modify_style(widget, style);
        g_object_unref(style);

	return false;
}

static void
block_prelight (Gtk::Widget& w)
{
	w.signal_enter_notify_event().connect (sigc::bind (sigc::ptr_fun (fixup_prelight), w.gobj()), false);
}

MonitorSection::MonitorSection (Session* s)
        : AxisView (s)
        , RouteUI (s)
        , main_table (2, 3)
        , _tearoff (0)
        , gain_adjustment (1.0, 0.0, 1.0, 0.01, 0.1)
        , gain_control (0)
        , dim_adjustment (0.2, 0.0, 1.0, 0.01, 0.1) 
        , dim_control (0)
        , solo_boost_adjustment (1.0, 1.0, 2.0, 0.01, 0.1) 
        , solo_boost_control (0)
        , solo_cut_adjustment (0.0, 0.0, 1.0, 0.01, 0.1)
        , solo_cut_control (0)
        , solo_in_place_button (solo_model_group, _("SiP"))
        , afl_button (solo_model_group, _("AFL"))
        , pfl_button (solo_model_group, _("PFL"))
        , cut_all_button (_("MUTE"))
        , dim_all_button (_("dim"))
        , mono_button (_("mono"))
        , rude_solo_button (_("soloing"))

{
        Glib::RefPtr<Action> act;

        if (!monitor_actions) {

                /* do some static stuff */

                register_actions ();

        }
        
        set_session (s);
        
        VBox* spin_packer;
        Label* spin_label;

        /* Dim */

        dim_control = new VolumeController (little_knob_pixbuf, &dim_adjustment, false, 30, 30);
        dim_adjustment.signal_value_changed().connect (sigc::mem_fun (*this, &MonitorSection::dim_level_changed));

        HBox* dim_packer = manage (new HBox);
        dim_packer->show ();

        spin_label = manage (new Label (_("Dim Cut")));
        spin_packer = manage (new VBox);
        spin_packer->show ();
        spin_packer->set_spacing (6);
        spin_packer->pack_start (*dim_control, false, false);
        spin_packer->pack_start (*spin_label, false, false); 

        dim_packer->set_spacing (12);
        dim_packer->pack_start (*spin_packer, true, true);

        /* Rude Solo */

	rude_solo_button.set_name ("TransportSoloAlert");
        rude_solo_button.show ();
        block_prelight (rude_solo_button);

        ARDOUR_UI::Blink.connect (sigc::mem_fun (*this, &MonitorSection::solo_blink));
	rude_solo_button.signal_button_press_event().connect (sigc::mem_fun(*this, &MonitorSection::cancel_solo), false);
        UI::instance()->set_tip (rude_solo_button, _("When active, something is soloed.\nClick to de-solo everything"));


        solo_model_box.set_spacing (6);
        solo_model_box.pack_start (solo_in_place_button, false, false);
        solo_model_box.pack_start (afl_button, false, false);
        solo_model_box.pack_start (pfl_button, false, false);

        solo_in_place_button.show ();
        afl_button.show ();
        pfl_button.show ();
        solo_model_box.show ();

        act = ActionManager::get_action (X_("Solo"), X_("solo-use-in-place"));
        if (act) {
                act->connect_proxy (solo_in_place_button);
        } 

        act = ActionManager::get_action (X_("Solo"), X_("solo-use-afl"));
        if (act) {
                act->connect_proxy (afl_button);
        } 

        act = ActionManager::get_action (X_("Solo"), X_("solo-use-pfl"));
        if (act) {
                act->connect_proxy (pfl_button);
        } 


        /* Solo Boost */

        solo_boost_control = new VolumeController (little_knob_pixbuf, &solo_boost_adjustment, false, 30, 30);
        solo_boost_adjustment.signal_value_changed().connect (sigc::mem_fun (*this, &MonitorSection::solo_boost_changed));

        HBox* solo_packer = manage (new HBox);
        solo_packer->set_spacing (12);
        solo_packer->show ();

        spin_label = manage (new Label (_("Solo Boost")));
        spin_packer = manage (new VBox);
        spin_packer->show ();
        spin_packer->set_spacing (6);
        spin_packer->pack_start (*solo_boost_control, false, false);
        spin_packer->pack_start (*spin_label, false, false); 

        solo_packer->pack_start (*spin_packer, true, true);

        /* Solo (SiP) cut */

        solo_cut_control = new VolumeController (little_knob_pixbuf, &solo_cut_adjustment, false, 30, 30);
        solo_cut_adjustment.signal_value_changed().connect (sigc::mem_fun (*this, &MonitorSection::solo_cut_changed));

        spin_label = manage (new Label (_("SiP Cut")));
        spin_packer = manage (new VBox);
        spin_packer->show ();
        spin_packer->set_spacing (6);
        spin_packer->pack_start (*solo_cut_control, false, false);
        spin_packer->pack_start (*spin_label, false, false); 

        solo_packer->pack_start (*spin_packer, true, true);

        upper_packer.set_spacing (12);
        upper_packer.pack_start (rude_solo_button, false, false);
        upper_packer.pack_start (solo_model_box, false, false);
        upper_packer.pack_start (*solo_packer, false, false);

        act = ActionManager::get_action (X_("Monitor"), X_("monitor-cut-all"));
        if (act) {
                act->connect_proxy (cut_all_button);
        } 

        act = ActionManager::get_action (X_("Monitor"), X_("monitor-dim-all"));
        if (act) {
                act->connect_proxy (dim_all_button);
        } 

        act = ActionManager::get_action (X_("Monitor"), X_("monitor-mono"));
        if (act) {
                act->connect_proxy (mono_button);
        } 

        cut_all_button.set_size_request (50,50);
        cut_all_button.show ();

        HBox* bbox = manage (new HBox);

        bbox->set_spacing (12);
        bbox->pack_start (mono_button, true, true);
        bbox->pack_start (dim_all_button, true, true);

        lower_packer.set_spacing (12);
        lower_packer.pack_start (*bbox, false, false);
        lower_packer.pack_start (cut_all_button, false, false);

        /* Gain */

        gain_control = new VolumeController (big_knob_pixbuf, &gain_adjustment, false, 80, 80);
        gain_adjustment.signal_value_changed().connect (sigc::mem_fun (*this, &MonitorSection::gain_value_changed));

        spin_label = manage (new Label (_("Gain")));
        spin_packer = manage (new VBox);
        spin_packer->show ();
        spin_packer->set_spacing (6);
        spin_packer->pack_start (*gain_control, false, false);
        spin_packer->pack_start (*spin_label, false, false);

        lower_packer.pack_start (*spin_packer, true, true);

        vpacker.set_border_width (12);
        vpacker.set_spacing (12);
        vpacker.pack_start (upper_packer, false, false);
        vpacker.pack_start (*dim_packer, false, false);
        vpacker.pack_start (main_table, false, false);
        vpacker.pack_start (lower_packer, false, false);

        hpacker.set_border_width (12);
        hpacker.set_spacing (12);
        hpacker.pack_start (vpacker, true, true);

        gain_control->show_all ();
        dim_control->show_all ();
        solo_boost_control->show_all ();

        main_table.show ();
        hpacker.show ();
        upper_packer.show ();
        lower_packer.show ();
        vpacker.show ();

        populate_buttons ();
        map_state ();

        _tearoff = new TearOff (hpacker);

        /* if torn off, make this a normal window */
        _tearoff->tearoff_window().set_type_hint (Gdk::WINDOW_TYPE_HINT_NORMAL);
        _tearoff->tearoff_window().set_title (X_("Monitor"));
        _tearoff->tearoff_window().signal_key_press_event().connect (sigc::ptr_fun (forward_key_press), false);
}

MonitorSection::~MonitorSection ()
{
        for (ChannelButtons::iterator i = _channel_buttons.begin(); i != _channel_buttons.end(); ++i) {
                delete *i;
        }

        _channel_buttons.clear ();

        delete gain_control;
        delete dim_control;
        delete solo_boost_control;
        delete _tearoff;
}

void
MonitorSection::set_session (Session* s)
{
        AxisView::set_session (s);

        if (_session) {

                _route = _session->monitor_out ();

                if (_route) {
                        /* session with control outs */
                        _monitor = _route->monitor_control ();
                } else { 
                        /* session with no control outs */
                        _monitor.reset ();
                        _route.reset ();
                }
                        
        } else {
                /* no session */
                _monitor.reset ();
                _route.reset ();
        }

        /* both might be null */
}

MonitorSection::ChannelButtonSet::ChannelButtonSet ()
        : cut (X_(""))
        , dim (X_(""))
        , solo (X_(""))
        , invert (X_(""))
{
        cut.set_name (X_("MixerMuteButton"));
        dim.set_name (X_("MixerMuteButton"));
        solo.set_name (X_("MixerSoloButton"));

        gtk_activatable_set_use_action_appearance (GTK_ACTIVATABLE (cut.gobj()), false);
        gtk_activatable_set_use_action_appearance (GTK_ACTIVATABLE (dim.gobj()), false);
        gtk_activatable_set_use_action_appearance (GTK_ACTIVATABLE (invert.gobj()), false);
        gtk_activatable_set_use_action_appearance (GTK_ACTIVATABLE (solo.gobj()), false);

        block_prelight (cut);
        block_prelight (dim);
        block_prelight (solo);
        block_prelight (invert);
}

void
MonitorSection::populate_buttons ()
{
        if (!_monitor) {
                return;
        }

        Glib::RefPtr<Action> act;
        uint32_t nchans = _monitor->output_streams().n_audio();
        
        main_table.resize (nchans+1, 5);
        main_table.set_col_spacings (6);
        main_table.set_row_spacings (6);
        main_table.set_homogeneous (true);

        Label* l1 = manage (new Label (X_("out")));
        main_table.attach (*l1, 0, 1, 0, 1, SHRINK|FILL, SHRINK|FILL);
        l1 = manage (new Label (X_("cut")));
        main_table.attach (*l1, 1, 2, 0, 1, SHRINK|FILL, SHRINK|FILL);
        l1 = manage (new Label (X_("dim")));
        main_table.attach (*l1, 2, 3, 0, 1, SHRINK|FILL, SHRINK|FILL);
        l1 = manage (new Label (X_("solo")));
        main_table.attach (*l1, 3, 4, 0, 1, SHRINK|FILL, SHRINK|FILL);
        l1 = manage (new Label (X_("inv")));
        main_table.attach (*l1, 4, 5, 0, 1, SHRINK|FILL, SHRINK|FILL);

        const uint32_t row_offset = 1;

        for (uint32_t i = 0; i < nchans; ++i) {
                
                string l;
                char buf[64];

                if (nchans == 2) {
                        if (i == 0) {
                                l = "L";
                        } else {
                                l = "R";
                        }
                } else {
                        char buf[32];
                        snprintf (buf, sizeof (buf), "%d", i+1);
                        l = buf;
                }

                Label* label = manage (new Label (l));
                main_table.attach (*label, 0, 1, i+row_offset, i+row_offset+1, SHRINK|FILL, SHRINK|FILL);

                ChannelButtonSet* cbs = new ChannelButtonSet;

                _channel_buttons.push_back (cbs);

                main_table.attach (cbs->cut, 1, 2, i+row_offset, i+row_offset+1, SHRINK|FILL, SHRINK|FILL);
                main_table.attach (cbs->dim, 2, 3, i+row_offset, i+row_offset+1, SHRINK|FILL, SHRINK|FILL); 
                main_table.attach (cbs->solo, 3, 4, i+row_offset, i+row_offset+1, SHRINK|FILL, SHRINK|FILL);
                main_table.attach (cbs->invert, 4, 5, i+row_offset, i+row_offset+1, SHRINK|FILL, SHRINK|FILL);
               
                snprintf (buf, sizeof (buf), "monitor-cut-%u", i+1);
                act = ActionManager::get_action (X_("Monitor"), buf);
                if (act) {
                        act->connect_proxy (cbs->cut);
                } 

                snprintf (buf, sizeof (buf), "monitor-dim-%u", i+1);
                act = ActionManager::get_action (X_("Monitor"), buf);
                if (act) {
                        act->connect_proxy (cbs->dim);
                }

                snprintf (buf, sizeof (buf), "monitor-solo-%u", i+1);
                act = ActionManager::get_action (X_("Monitor"), buf);
                if (act) {
                        act->connect_proxy (cbs->solo);
                }

                snprintf (buf, sizeof (buf), "monitor-invert-%u", i+1);
                act = ActionManager::get_action (X_("Monitor"), buf);
                if (act) {
                        act->connect_proxy (cbs->invert);
                }
        }

        main_table.show_all ();
}

void 
MonitorSection::set_button_names ()
{
        rec_enable_button_label.set_text ("rec");
        mute_button_label.set_text ("rec");
        solo_button_label.set_text ("rec");
}

void
MonitorSection::dim_all ()
{
        if (!_monitor) {
                return;
        }

        Glib::RefPtr<Action> act = ActionManager::get_action (X_("Monitor"), "monitor-dim-all");
        if (act) {
		Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic(act);
                _monitor->set_dim_all (tact->get_active());
        }

}

void
MonitorSection::cut_all ()
{
        if (!_monitor) {
                return;
        }

        Glib::RefPtr<Action> act = ActionManager::get_action (X_("Monitor"), "monitor-cut-all");
        if (act) {
		Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic(act);
                _monitor->set_cut_all (tact->get_active());
        }
}

void
MonitorSection::mono ()
{
        if (!_monitor) {
                return;
        }

        Glib::RefPtr<Action> act = ActionManager::get_action (X_("Monitor"), "monitor-mono");
        if (act) {
		Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic(act);
                _monitor->set_mono (tact->get_active());
        }
}

void
MonitorSection::cut_channel (uint32_t chn)
{
        if (!_monitor) {
                return;
        }

        char buf[64];
        snprintf (buf, sizeof (buf), "monitor-cut-%u", chn);

        --chn; // 0-based in backend

        Glib::RefPtr<Action> act = ActionManager::get_action (X_("Monitor"), buf);
        if (act) {
		Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic(act);
                _monitor->set_cut (chn, tact->get_active());
        }
}

void
MonitorSection::dim_channel (uint32_t chn)
{
        if (!_monitor) {
                return;
        }

        char buf[64];
        snprintf (buf, sizeof (buf), "monitor-dim-%u", chn);

        --chn; // 0-based in backend

        Glib::RefPtr<Action> act = ActionManager::get_action (X_("Monitor"), buf);
        if (act) {
		Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic(act);
                _monitor->set_dim (chn, tact->get_active());
        }

}

void
MonitorSection::solo_channel (uint32_t chn)
{
        if (!_monitor) {
                return;
        }

        char buf[64];
        snprintf (buf, sizeof (buf), "monitor-solo-%u", chn);

        --chn; // 0-based in backend

        Glib::RefPtr<Action> act = ActionManager::get_action (X_("Monitor"), buf);
        if (act) {
		Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic(act);
                _monitor->set_solo (chn, tact->get_active());
        }

}

void
MonitorSection::invert_channel (uint32_t chn)
{
        if (!_monitor) {
                return;
        }

        char buf[64];
        snprintf (buf, sizeof (buf), "monitor-invert-%u", chn);

        --chn; // 0-based in backend

        Glib::RefPtr<Action> act = ActionManager::get_action (X_("Monitor"), buf);
        if (act) {
		Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic(act);
                _monitor->set_polarity (chn, tact->get_active());
        } 
}

void
MonitorSection::register_actions ()
{
        string action_name;
        string action_descr;

        monitor_actions = ActionGroup::create (X_("Monitor"));
	ActionManager::add_action_group (monitor_actions);

        ActionManager::register_toggle_action (monitor_actions, "monitor-mono", "", 
                                               sigc::mem_fun (*this, &MonitorSection::mono));

        ActionManager::register_toggle_action (monitor_actions, "monitor-cut-all", "", 
                                               sigc::mem_fun (*this, &MonitorSection::cut_all));

        ActionManager::register_toggle_action (monitor_actions, "monitor-dim-all", "", 
                                               sigc::mem_fun (*this, &MonitorSection::dim_all));

        /* note the 1-based counting (for naming - backend uses 0-based) */

        for (uint32_t chn = 1; chn <= 16; ++chn) {

                /* for the time being, do not use the action description because it always
                   shows up in the buttons, which is undesirable.
                */

                action_name = string_compose (X_("monitor-cut-%1"), chn);
                action_descr = string_compose (_("Cut Monitor Chn %1"), chn);
                ActionManager::register_toggle_action (monitor_actions, action_name.c_str(), "", 
                                                       sigc::bind (sigc::mem_fun (*this, &MonitorSection::cut_channel), chn));

                action_name = string_compose (X_("monitor-dim-%1"), chn);
                action_descr = string_compose (_("Dim Monitor Chn %1"), chn+1);
                ActionManager::register_toggle_action (monitor_actions, action_name.c_str(), "", 
                                                       sigc::bind (sigc::mem_fun (*this, &MonitorSection::dim_channel), chn));

                action_name = string_compose (X_("monitor-solo-%1"), chn);
                action_descr = string_compose (_("Solo Monitor Chn %1"), chn);
                ActionManager::register_toggle_action (monitor_actions, action_name.c_str(), "", 
                                                       sigc::bind (sigc::mem_fun (*this, &MonitorSection::solo_channel), chn));

                action_name = string_compose (X_("monitor-invert-%1"), chn);
                action_descr = string_compose (_("Invert Monitor Chn %1"), chn);
                ActionManager::register_toggle_action (monitor_actions, action_name.c_str(), "", 
                                                       sigc::bind (sigc::mem_fun (*this, &MonitorSection::invert_channel), chn));

        }


        Glib::RefPtr<ActionGroup> solo_actions = ActionGroup::create (X_("Solo"));
        RadioAction::Group solo_group;

        ActionManager::register_radio_action (solo_actions, solo_group, "solo-use-in-place", "",
                                              sigc::mem_fun (*this, &MonitorSection::solo_use_in_place));
        ActionManager::register_radio_action (solo_actions, solo_group, "solo-use-afl", "",
                                              sigc::mem_fun (*this, &MonitorSection::solo_use_afl));
        ActionManager::register_radio_action (solo_actions, solo_group, "solo-use-pfl", "",
                                              sigc::mem_fun (*this, &MonitorSection::solo_use_pfl));

	ActionManager::add_action_group (solo_actions);
}

void
MonitorSection::solo_use_in_place ()
{
	/* this is driven by a toggle on a radio group, and so is invoked twice,
	   once for the item that became inactive and once for the one that became
	   active.
	*/

        Glib::RefPtr<Action> act = ActionManager::get_action (X_("Solo"), X_("solo-use-in-place"));

        if (act) {
                Glib::RefPtr<RadioAction> ract = Glib::RefPtr<RadioAction>::cast_dynamic (act);
                if (ract) {
                        Config->set_solo_control_is_listen_control (!ract->get_active());
                }
        }
}

void
MonitorSection::solo_use_afl ()
{
	/* this is driven by a toggle on a radio group, and so is invoked twice,
	   once for the item that became inactive and once for the one that became
	   active.
	*/
        
        Glib::RefPtr<Action> act = ActionManager::get_action (X_("Solo"), X_("solo-use-afl"));
        if (act) {
                Glib::RefPtr<RadioAction> ract = Glib::RefPtr<RadioAction>::cast_dynamic (act);
                if (ract) {
                        if (ract->get_active()) {
                                Config->set_listen_position (AfterFaderListen);
                                Config->set_solo_control_is_listen_control (true);
                        }
                }
        }
}

void
MonitorSection::solo_use_pfl ()
{
	/* this is driven by a toggle on a radio group, and so is invoked twice,
	   once for the item that became inactive and once for the one that became
	   active.
	*/

        Glib::RefPtr<Action> act = ActionManager::get_action (X_("Solo"), X_("solo-use-afl"));
        if (act) {
                Glib::RefPtr<RadioAction> ract = Glib::RefPtr<RadioAction>::cast_dynamic (act);
                if (ract) {
                        if (ract->get_active()) {
                                Config->set_listen_position (PreFaderListen);
                                Config->set_solo_control_is_listen_control (true);
                        }
                }
        }
}

void
MonitorSection::setup_knob_images ()
{
        try {
                
                big_knob_pixbuf = ::get_icon ("bigknob");
                
        }  catch (...) {
                
                error << "No knob image found (or not loadable) at "
                      << " .... "
                      << endmsg;
                throw failed_constructor ();
        }
        
        try {
                
                little_knob_pixbuf = ::get_icon ("littleknob");
                
        }  catch (...) {
                
                error << "No knob image found (or not loadable) at "
                      << " .... "
                      << endmsg;
                throw failed_constructor ();
        }
}

void
MonitorSection::gain_value_changed ()
{
        if (_route) {
                _route->set_gain (slider_position_to_gain (gain_adjustment.get_value()), this);
        }
}

void
MonitorSection::dim_level_changed ()
{
        if (_monitor) {
                _monitor->set_dim_level (dim_adjustment.get_value());
        }
}

void
MonitorSection::solo_boost_changed ()
{
        if (_monitor) {
                _monitor->set_solo_boost_level (solo_boost_adjustment.get_value());
        }
}

bool
MonitorSection::nonlinear_gain_printer (SpinButton* button)
{
        double val = button->get_adjustment()->get_value();
        char buf[16];
        snprintf (buf, sizeof (buf), "%.1f", accurate_coefficient_to_dB (slider_position_to_gain (val)));
        button->set_text (buf);
        return true;
}

bool
MonitorSection::linear_gain_printer (SpinButton* button)
{
        double val = button->get_adjustment()->get_value();
        char buf[16];
        snprintf (buf, sizeof (buf), "%.1f", accurate_coefficient_to_dB (val));
        button->set_text (buf);
        return true;
}

void
MonitorSection::map_state ()
{
        if (!_route || !_monitor) {
                return;
        }

        gain_control->get_adjustment()->set_value (gain_to_slider_position (_route->gain_control()->get_value()));
        dim_control->get_adjustment()->set_value (_monitor->dim_level());
        solo_boost_control->get_adjustment()->set_value (_monitor->solo_boost_level());

        const char *action_name;

        if (Config->get_solo_control_is_listen_control()) {
		switch (Config->get_listen_position()) {
		case AfterFaderListen:
                        action_name = X_("solo-use-afl");
			break;
		case PreFaderListen:
                        action_name = X_("solo-use-afl");
			break;
		}
        } else {
                action_name = X_("solo-use-in-place");
        }
        
        Glib::RefPtr<Action> act;
        
        act = ActionManager::get_action (X_("Solo"), action_name);
        if (act) {
                Glib::RefPtr<RadioAction> ract = Glib::RefPtr<RadioAction>::cast_dynamic (act);
                if (ract) {
                        ract->set_active (true);
                }
        }

        act = ActionManager::get_action (X_("Monitor"), "monitor-cut-all");
        if (act) {
                Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic (act);
                if (tact) {
                        cerr << "Set monitor cut all action to " << _monitor->cut_all () << endl;
                        tact->set_active (_monitor->cut_all());
                } else {
                        cerr << " no global cut action\n";
                }
        } else {
                cerr << " no global cut action2\n";
        }

        act = ActionManager::get_action (X_("Monitor"), "monitor-dim-all");
        if (act) {
                Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic (act);
                if (tact) {
                        tact->set_active (_monitor->dim_all());
                }
        }
        
        act = ActionManager::get_action (X_("Monitor"), "monitor-mono");
        if (act) {
                Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic (act);
                if (tact) {
                        tact->set_active (_monitor->mono());
                }
        }

        uint32_t nchans = _monitor->output_streams().n_audio();

        assert (nchans == _channel_buttons.size ());

        for (uint32_t n = 0; n < nchans; ++n) {

                char action_name[32];

                snprintf (action_name, sizeof (action_name), "monitor-cut-%u", n+1);
                act = ActionManager::get_action (X_("Monitor"), action_name);
                if (act) {
                        Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic (act);
                        if (tact) {
                                tact->set_active (_monitor->cut (n));
                        }
                }

                snprintf (action_name, sizeof (action_name), "monitor-dim-%u", n+1);
                act = ActionManager::get_action (X_("Monitor"), action_name);
                if (act) {
                        Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic (act);
                        if (tact) {
                                tact->set_active (_monitor->dimmed (n));
                        }
                }

                snprintf (action_name, sizeof (action_name), "monitor-solo-%u", n+1);
                act = ActionManager::get_action (X_("Monitor"), action_name);
                if (act) {
                        Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic (act);
                        if (tact) {
                                tact->set_active (_monitor->soloed (n));
                        }
                }

                snprintf (action_name, sizeof (action_name), "monitor-invert-%u", n+1);
                act = ActionManager::get_action (X_("Monitor"), action_name);
                if (act) {
                        Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic (act);
                        if (tact) {
                                tact->set_active (_monitor->inverted (n));
                        }
                }
        }
}

void
MonitorSection::solo_blink (bool onoff)
{
	if (_session == 0) {
		return;
	}

	if (_session->soloing() || _session->listening()) {
		if (onoff) {
			rude_solo_button.set_state (STATE_ACTIVE);
		} else {
			rude_solo_button.set_state (STATE_NORMAL);
		}
	} else {
		// rude_solo_button.set_active (false);
		rude_solo_button.set_state (STATE_NORMAL);
	}
}

bool
MonitorSection::cancel_solo (GdkEventButton* ev)
{
        if (_session) {
                if (_session->soloing()) {
                        _session->set_solo (_session->get_routes(), false);
                } else if (_session->listening()) {
                        _session->set_listen (_session->get_routes(), false);
                }
        }

        return true;
}

void
MonitorSection::solo_cut_changed ()
{
        Config->set_solo_mute_gain (slider_position_to_gain (solo_cut_adjustment.get_value()));
}