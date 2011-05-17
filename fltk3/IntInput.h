//
// "$Id: IntInput.h 8022 2010-12-12 23:21:03Z AlbrechtS $"
//
// Integer input header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

/* \file
   Fl_Int_Input widget . */

#ifndef Fltk3_Int_Input_H
#define Fltk3_Int_Input_H

#include "Input.h"

/**
  The Fl_Int_Input class is a subclass of Fl_Input
  that only allows the user to type decimal digits (or hex numbers of the form 0xaef).
*/
class FLTK3_EXPORT Fl_Int_Input : public Fl_Input {
public:
  /**
    Creates a new Fl_Int_Input widget using the given position,
    size, and label string. The default boxtype is fltk3::DOWN_BOX.
    <P>Inherited destructor Destroys the widget and any value associated with it.
  */
  Fl_Int_Input(int X,int Y,int W,int H,const char *l = 0)
      : Fl_Input(X,Y,W,H,l) {type(FL_INT_INPUT);}
};

#endif

//
// End of "$Id: IntInput.h 8022 2010-12-12 23:21:03Z AlbrechtS $".
//
