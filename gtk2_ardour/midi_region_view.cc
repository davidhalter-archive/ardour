/*
    Copyright (C) 2001-2007 Paul Davis 
    Author: Dave Robillard

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <cmath>
#include <cassert>
#include <algorithm>

#include <gtkmm.h>

#include <gtkmm2ext/gtk_ui.h>

#include <sigc++/signal.h>

#include <ardour/playlist.h>
#include <ardour/tempo.h>
#include <ardour/midi_region.h>
#include <ardour/midi_source.h>
#include <ardour/midi_diskstream.h>
#include <ardour/midi_events.h>
#include <ardour/midi_model.h>

#include "streamview.h"
#include "midi_region_view.h"
#include "midi_streamview.h"
#include "midi_time_axis.h"
#include "simplerect.h"
#include "simpleline.h"
#include "canvas-hit.h"
#include "public_editor.h"
#include "ghostregion.h"
#include "midi_time_axis.h"
#include "utils.h"
#include "rgb_macros.h"
#include "gui_thread.h"

#include "i18n.h"

using namespace sigc;
using namespace ARDOUR;
using namespace PBD;
using namespace Editing;
using namespace ArdourCanvas;

MidiRegionView::MidiRegionView (ArdourCanvas::Group *parent, RouteTimeAxisView &tv, boost::shared_ptr<MidiRegion> r, double spu,
				  Gdk::Color& basic_color)
	: RegionView (parent, tv, r, spu, basic_color)
	, _default_note_length(0.0)
	, _active_notes(0)
	, _delta_command(NULL)
	, _command_mode(None)
{
}

MidiRegionView::MidiRegionView (ArdourCanvas::Group *parent, RouteTimeAxisView &tv, boost::shared_ptr<MidiRegion> r, double spu, 
				  Gdk::Color& basic_color, TimeAxisViewItem::Visibility visibility)
	: RegionView (parent, tv, r, spu, basic_color, visibility)
	, _default_note_length(0.0)
	, _active_notes(0)
	, _delta_command(NULL)
	, _command_mode(None)
{
}

void
MidiRegionView::init (Gdk::Color& basic_color, bool wfd)
{
	if (wfd)
		midi_region()->midi_source(0)->load_model();
	
	const Meter& m = trackview.session().tempo_map().meter_at(_region->start());
	const Tempo& t = trackview.session().tempo_map().tempo_at(_region->start());
	_default_note_length = m.frames_per_bar(t, trackview.session().frame_rate())
			/ m.beats_per_bar();

	_model = midi_region()->midi_source(0)->model();
	_enable_display = false;
	
	RegionView::init(basic_color, false);

	compute_colors (basic_color);

	reset_width_dependent_items ((double) _region->length() / samples_per_unit);

	set_y_position_and_height (0, trackview.height);

	region_muted ();
	region_resized (BoundsChanged);
	region_locked ();

	_region->StateChanged.connect (mem_fun(*this, &MidiRegionView::region_changed));

	set_colors ();

	_enable_display = true;

	if (_model) {
		if (wfd) {
			redisplay_model();
		}
		_model->ContentsChanged.connect(sigc::mem_fun(this, &MidiRegionView::redisplay_model));
	}
	
	group->signal_event().connect (mem_fun (this, &MidiRegionView::canvas_event), false);

}

bool
MidiRegionView::canvas_event(GdkEvent* ev)
{
	enum State { None, Pressed, SelectDragging, AddDragging };
	static int press_button = 0;
	static State _state;

	static double drag_start_x, drag_start_y;
	static double last_x, last_y;
	double event_x, event_y;

	static ArdourCanvas::SimpleRect* drag_rect = NULL;
	
	if (trackview.editor.current_mouse_mode() != MouseNote)
		return false;

	switch (ev->type) {
	case GDK_KEY_PRESS:
		if (ev->key.keyval == GDK_Delete)
			start_remove_command();
		break;
	
	case GDK_KEY_RELEASE:
		if (_command_mode == Remove && ev->key.keyval == GDK_Delete)
			apply_command();
		break;

	case GDK_BUTTON_PRESS:
		//group->grab(GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK, ev->button.time);
		_state = Pressed;
		press_button = ev->button.button;
		//cerr << "PRESSED: " << press_button << endl;
		return true;
	
	case GDK_MOTION_NOTIFY:
		event_x = ev->motion.x;
		event_y = ev->motion.y;
		group->w2i(event_x, event_y);

		switch (_state) {
		case Pressed: // Drag start

			if (press_button == 1 && _command_mode != Remove) { // Select rect start
				group->grab(GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
						Gdk::Cursor(Gdk::FLEUR), ev->motion.time);
				last_x = event_x;
				last_y = event_y;
				drag_start_x = event_x;
				drag_start_y = event_y;

				drag_rect = new ArdourCanvas::SimpleRect(*group);
				drag_rect->property_x1() = event_x;
				drag_rect->property_y1() = event_y;
				drag_rect->property_x2() = event_x;
				drag_rect->property_y2() = event_y;
				drag_rect->property_outline_what() = 0xFF;
				drag_rect->property_outline_color_rgba() = 0xFF000099;
				drag_rect->property_fill_color_rgba() = 0xFFDDDD33;

				_state = SelectDragging;
				return true;

			} else if (press_button == 3) { // Add note drag start
				group->grab(GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
						Gdk::Cursor(Gdk::FLEUR), ev->motion.time);
				last_x = event_x;
				last_y = event_y;
				drag_start_x = event_x;
				drag_start_y = event_y;
				
				nframes_t event_frame = midi_view()->editor.pixel_to_frame(event_x);
				midi_view()->editor.snap_to(event_frame);

				drag_rect = new ArdourCanvas::SimpleRect(*group);
				drag_rect->property_x1() = midi_view()->editor.frame_to_pixel(event_frame);
				
				drag_rect->property_y1() = midi_stream_view()->note_to_y(midi_stream_view()->y_to_note(event_y));
				drag_rect->property_x2() = event_x;
				drag_rect->property_y2() = drag_rect->property_y1() + floor(midi_stream_view()->note_height());
				drag_rect->property_outline_what() = 0xFF;
				drag_rect->property_outline_color_rgba() = 0xFFFFFF99;
				drag_rect->property_fill_color_rgba() = 0xFFFFFF66;

				_state = AddDragging;
				return true;
			}

			break;

		case SelectDragging: // Select rect motion
		case AddDragging: // Add note rect motion
			if (ev->motion.is_hint) {
				int t_x;
				int t_y;
				GdkModifierType state;
				gdk_window_get_pointer(ev->motion.window, &t_x, &t_y, &state);
				event_x = t_x;
				event_y = t_y;
			}

			if (_state == AddDragging) {
				nframes_t event_frame = midi_view()->editor.pixel_to_frame(event_x);
				midi_view()->editor.snap_to(event_frame);
				event_x = midi_view()->editor.frame_to_pixel(event_frame);
			}

			if (drag_rect)
				drag_rect->property_x2() = event_x;

			if (drag_rect && _state == SelectDragging)
				drag_rect->property_y2() = event_y;

			last_x = event_x;
			last_y = event_y;

			return true;
		default:
			_state = None;
			break;
		}
		break;
	
	case GDK_BUTTON_RELEASE:
		event_x = ev->motion.x;
		event_y = ev->motion.y;
		group->w2i(event_x, event_y);
		group->ungrab(ev->button.time);
		switch (_state) {
		case Pressed: // Clicked
			if (ev->button.button == 3) {
				nframes_t event_frame = midi_view()->editor.pixel_to_frame(event_x);
				midi_view()->editor.snap_to(event_frame);
				event_x = midi_view()->editor.frame_to_pixel(event_frame);
				create_note_at(event_x, event_y, _default_note_length);
			}
			_state = None;
			return true;
		case SelectDragging: // Select drag done
			_state = None;
			delete drag_rect;
			drag_rect = NULL;
			return true;
		case AddDragging: // Add drag done
			_state = None;
			if (drag_rect->property_x2() > drag_rect->property_x1() + 2) {
				create_note_at(drag_rect->property_x1(), drag_rect->property_y1(),
						trackview.editor.pixel_to_frame(
						drag_rect->property_x2() - drag_rect->property_x1()));
			}

			delete drag_rect;
			drag_rect = NULL;
			return true;
		default:
			break;
		}
	
	default:
		break;
	}

	return false;
}


/** Add a note to the model, and the view, at a canvas (click) coordinate */
void
MidiRegionView::create_note_at(double x, double y, double dur)
{
	MidiTimeAxisView* const mtv = dynamic_cast<MidiTimeAxisView*>(&trackview);
	MidiStreamView* const view = mtv->midi_view();

	double note = midi_stream_view()->y_to_note(y);

	assert(note >= 0.0);
	assert(note <= 127.0);

	const nframes_t stamp = trackview.editor.pixel_to_frame (x);
	assert(stamp >= 0);
	//assert(stamp <= _region->length());

	//const Meter& m = trackview.session().tempo_map().meter_at(stamp);
	//const Tempo& t = trackview.session().tempo_map().tempo_at(stamp);
	//double dur = m.frames_per_bar(t, trackview.session().frame_rate()) / m.beats_per_bar();

	// Add a 1 beat long note (for now)
	const MidiModel::Note new_note(stamp, dur, (uint8_t)note, 0x40);
	
	view->update_bounds(new_note.note());

	MidiModel::DeltaCommand* cmd = _model->new_delta_command("add note");
	cmd->add(new_note);
	_model->apply_command(cmd);
	
	//add_note(new_note);
}


void
MidiRegionView::clear_events()
{
	for (std::vector<ArdourCanvas::Item*>::iterator i = _events.begin(); i != _events.end(); ++i)
		delete *i;
	
	_events.clear();
}


void
MidiRegionView::display_model(boost::shared_ptr<MidiModel> model)
{
	_model = model;

	if (_enable_display)
		redisplay_model();
}


void
MidiRegionView::redisplay_model()
{
	clear_events();
	
	if (_model) {
		begin_write();

		for (size_t i=0; i < _model->n_notes(); ++i)
			add_note(_model->note_at(i));

		end_write();
	} else {
		warning << "MidiRegionView::redisplay_model called without a model" << endmsg;
	}
}


MidiRegionView::~MidiRegionView ()
{
	in_destructor = true;
	end_write();

	RegionViewGoingAway (this); /* EMIT_SIGNAL */
}


void
MidiRegionView::region_resized (Change what_changed)
{
	RegionView::region_resized(what_changed);

	if (what_changed & ARDOUR::PositionChanged) {
	
		if (_enable_display)
			redisplay_model();
	
	} else if (what_changed & Change (StartChanged)) {

		//cerr << "MIDI RV START CHANGED" << endl;

	} else if (what_changed & Change (LengthChanged)) {
		
		//cerr << "MIDI RV LENGTH CHANGED" << endl;
	
	}
}

void
MidiRegionView::reset_width_dependent_items (double pixel_width)
{
	RegionView::reset_width_dependent_items(pixel_width);
	assert(_pixel_width == pixel_width);
		
	if (_enable_display)
		redisplay_model();
}

void
MidiRegionView::set_y_position_and_height (double y, double h)
{
	RegionView::set_y_position_and_height(y, h - 1);

	if (_enable_display)
		redisplay_model();

	if (name_text) {
		name_text->raise_to_top();
	}
}

void
MidiRegionView::show_region_editor ()
{
	cerr << "No MIDI region editor." << endl;
}

GhostRegion*
MidiRegionView::add_ghost (AutomationTimeAxisView& atv)
{
	RouteTimeAxisView* rtv = dynamic_cast<RouteTimeAxisView*>(&trackview);
	assert(rtv);

	double unit_position = _region->position () / samples_per_unit;
	GhostRegion* ghost = new GhostRegion (atv, unit_position);

	ghost->set_height ();
	ghost->set_duration (_region->length() / samples_per_unit);
	ghosts.push_back (ghost);

	ghost->GoingAway.connect (mem_fun(*this, &MidiRegionView::remove_ghost));

	return ghost;
}


/** Begin tracking note state for successive calls to add_event
 */
void
MidiRegionView::begin_write()
{
	_active_notes = new CanvasNote*[128];
	for (unsigned i=0; i < 128; ++i)
		_active_notes[i] = NULL;
}


/** Destroy note state for add_event
 */
void
MidiRegionView::end_write()
{
	delete[] _active_notes;
	_active_notes = NULL;
}


/** Add a MIDI event.
 *
 * This is used while recording, and handles displaying still-unresolved notes.
 * Displaying an existing model is simpler, and done with add_note.
 */
void
MidiRegionView::add_event (const MidiEvent& ev)
{
	/*printf("Event, time = %f, size = %zu, data = ", ev.time, ev.size);
	for (size_t i=0; i < ev.size; ++i) {
		printf("%X ", ev.buffer[i]);
	}
	printf("\n\n");*/

	ArdourCanvas::Group* const group = (ArdourCanvas::Group*)get_canvas_group();
	
	if (midi_view()->note_mode() == Sustained) {
		if ((ev.buffer()[0] & 0xF0) == MIDI_CMD_NOTE_ON) {
			const Byte& note = ev.buffer()[1];
			const double y1 = midi_stream_view()->note_to_y(note);

			CanvasNote* ev_rect = new CanvasNote(*this, *group);
			ev_rect->property_x1() = trackview.editor.frame_to_pixel (
					(nframes_t)ev.time());
			ev_rect->property_y1() = y1;
			ev_rect->property_x2() = trackview.editor.frame_to_pixel (
					_region->length());
			ev_rect->property_y2() = y1 + floor(midi_stream_view()->note_height());
			ev_rect->property_outline_color_rgba() = 0xFFFFFFAA;
			/* outline all but right edge */
			ev_rect->property_outline_what() = (guint32) (0x1 & 0x4 & 0x8);
			ev_rect->property_fill_color_rgba() = 0xFFFFFF66;

			ev_rect->raise_to_top();

			_events.push_back(ev_rect);
			if (_active_notes)
				_active_notes[note] = ev_rect;

		} else if ((ev.buffer()[0] & 0xF0) == MIDI_CMD_NOTE_OFF) {
			const Byte& note = ev.buffer()[1];
			if (_active_notes && _active_notes[note]) {
				_active_notes[note]->property_x2() = trackview.editor.frame_to_pixel((nframes_t)ev.time());
				_active_notes[note]->property_outline_what() = (guint32) 0xF; // all edges
				_active_notes[note] = NULL;
			}
		}
	
	} else if (midi_view()->note_mode() == Percussive) {
		const Byte& note = ev.buffer()[1];
		const double diamond_size = midi_stream_view()->note_height() / 2.0;
		const double x = trackview.editor.frame_to_pixel((nframes_t)ev.time());
		const double y = midi_stream_view()->note_to_y(note) + ((diamond_size-2) / 4.0);

		CanvasHit* ev_diamond = new CanvasHit(*this, *group, diamond_size);
		ev_diamond->move(x, y);
		ev_diamond->show();
		ev_diamond->property_outline_color_rgba() = 0xFFFFFFDD;
		ev_diamond->property_fill_color_rgba() = 0xFFFFFF66;
			
		_events.push_back(ev_diamond);
	}
}


/** Extend active notes to rightmost edge of region (if length is changed)
 */
void
MidiRegionView::extend_active_notes()
{
	if (!_active_notes)
		return;

	for (unsigned i=0; i < 128; ++i)
		if (_active_notes[i])
			_active_notes[i]->property_x2() = trackview.editor.frame_to_pixel(_region->length());
}


/** Add a MIDI note to the view (with duration).
 *
 * This does no 'realtime' note resolution, notes from a MidiModel have a
 * duration so they can be drawn in full immediately.
 */
void
MidiRegionView::add_note (const MidiModel::Note& note)
{
	assert(note.time() >= 0);
	assert(note.time() < _region->length());
	//assert(note.time() + note.duration < _region->length());

	ArdourCanvas::Group* const group = (ArdourCanvas::Group*)get_canvas_group();
	
	const uint8_t fill_alpha = 0x20 + (uint8_t)(note.velocity() * 1.5); 
	const uint32_t fill = RGBA_TO_UINT(0xE0 + note.velocity()/127.0 * 0x10, 0xE0, 0xE0, fill_alpha);
	const uint8_t outline_alpha = 0x80 + (uint8_t)(note.velocity()); 
	const uint32_t outline = RGBA_TO_UINT(0xE0 + note.velocity()/127.0 * 0x10, 0xE0, 0xE0, outline_alpha);
	
	//printf("Event, time = %f, note = %d\n", note.time(), note.note());

	if (midi_view()->note_mode() == Sustained) {
		const double y1 = midi_stream_view()->note_to_y(note.note());

		ArdourCanvas::SimpleRect * ev_rect = new CanvasNote(*this, *group, &note);
		ev_rect->property_x1() = trackview.editor.frame_to_pixel((nframes_t)note.time());
		ev_rect->property_y1() = y1;
		ev_rect->property_x2() = trackview.editor.frame_to_pixel((nframes_t)(note.end_time()));
		ev_rect->property_y2() = y1 + floor(midi_stream_view()->note_height());

		ev_rect->property_fill_color_rgba() = fill;
		ev_rect->property_outline_color_rgba() = outline;
		ev_rect->property_outline_what() = (guint32) 0xF; // all edges

		ev_rect->show();
		_events.push_back(ev_rect);

	} else if (midi_view()->note_mode() == Percussive) {
		const double diamond_size = midi_stream_view()->note_height() / 2.0;
		const double x = trackview.editor.frame_to_pixel((nframes_t)note.time());
		const double y = midi_stream_view()->note_to_y(note.note()) + ((diamond_size-2) / 4.0);

		CanvasHit* ev_diamond = new CanvasHit(*this, *group, diamond_size);
		ev_diamond->move(x, y);
		ev_diamond->show();
		ev_diamond->property_fill_color_rgba() = fill;
		ev_diamond->property_outline_color_rgba() = outline;
		_events.push_back(ev_diamond);
	}
}


