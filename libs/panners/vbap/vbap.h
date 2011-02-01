/*
    Copyright (C) 2010 Paul Davis

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

#ifndef __libardour_vbap_h__
#define __libardour_vbap_h__

#include <string>
#include <map>

#include "pbd/cartesian.h"

#include "ardour/panner.h"
#include "ardour/panner_shell.h"

#include "vbap_speakers.h"

namespace ARDOUR {

class Speakers;
class Pannable;

class VBAPanner : public Panner 
{ 
public:
	VBAPanner (boost::shared_ptr<Pannable>, Speakers& s);
	~VBAPanner ();

        void configure_io (ChanCount in, ChanCount /* ignored - we use Speakers */);
        ChanCount in() const;
        ChanCount out() const;

        std::set<Evoral::Parameter> what_can_be_automated() const;

	static Panner* factory (boost::shared_ptr<Pannable>, Speakers& s);

	void distribute (BufferSet& ibufs, BufferSet& obufs, gain_t gain_coeff, pframes_t nframes);

	void set_azimuth_elevation (double azimuth, double elevation);

        std::string describe_parameter (Evoral::Parameter);
        std::string value_as_string (boost::shared_ptr<AutomationControl>) const;

	XMLNode& state (bool full_state);
	XMLNode& get_state ();
	int set_state (const XMLNode&, int version);


private:
        struct Signal {
            PBD::AngularVector direction;
            double gains[3];
            double desired_gains[3];
            int    outputs[3];
            int    desired_outputs[3];

            Signal (Session&, VBAPanner&, uint32_t which);
        };

        std::vector<Signal*> _signals;
        VBAPSpeakers&       _speakers;
        
	void compute_gains (double g[3], int ls[3], int azi, int ele);
        void update ();
        void clear_signals ();

	void distribute_one (AudioBuffer& src, BufferSet& obufs, gain_t gain_coeff, pframes_t nframes, uint32_t which);
	void distribute_one_automated (AudioBuffer& src, BufferSet& obufs,
                                          framepos_t start, framepos_t end, pframes_t nframes, 
                                          pan_t** buffers, uint32_t which);
};

} /* namespace */

#endif /* __libardour_vbap_h__ */