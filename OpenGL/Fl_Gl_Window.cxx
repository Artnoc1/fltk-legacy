//
// "$Id: Fl_Gl_Window.cxx,v 1.11 2000/05/01 17:25:15 carl Exp $"
//
// OpenGL window code for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-1999 by Bill Spitzak and others.
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
// Please report all bugs and problems to "fltk-bugs@easysw.com".
//

#include <config.h>
#if HAVE_GL

#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/Fl_Gl_Window.H>
#include "Fl_Gl_Choice.H"

////////////////////////////////////////////////////////////////

// The symbol SWAP_TYPE defines what is in the back buffer after doing
// a glXSwapBuffers().

// The OpenGl documentation says that the contents of the backbuffer
// are "undefined" after glXSwapBuffers().  However, if we know what
// is in the backbuffers then we can save a good deal of time.  For
// this reason you can define some symbols to describe what is left in
// the back buffer.

// The default of SWAP_SWAP works on an SGI, and will also work (but
// is sub-optimal) on machines that should be SWAP_COPY or SWAP_NODAMAGE.
// The win32 emulation of OpenGL can use COPY, but some (all?) OpenGL
// cards use SWAP.

// contents of back buffer after glXSwapBuffers():
#define UNDEFINED 0 	// unknown
#define SWAP 1		// former front buffer
#define COPY 2		// unchanged
#define NODAMAGE 3	// unchanged even by X expose() events

#ifdef MESA
#define SWAP_TYPE NODAMAGE
#else
#define SWAP_TYPE SWAP
#endif

////////////////////////////////////////////////////////////////

int Fl_Gl_Window::can_do(int a, const int *b) {
  return Fl_Gl_Choice::find(a,b) != 0;
}

void Fl_Gl_Window::create() {
  if (!g) {
    g = Fl_Gl_Choice::find(mode_, alist);
    if (!g) {Fl::error("Insufficient OpenGL"); return;}
  }
#ifndef _WIN32
  Fl_X::create(this, g->vis, g->colormap, -1);
  //if (overlay && overlay != this) ((Fl_Gl_Window*)overlay)->show();
#else
  Fl_Window::create();
#endif
}

void Fl_Gl_Window::invalidate() {
  valid(0);
#ifndef _WIN32
  if (overlay) ((Fl_Gl_Window*)overlay)->valid(0);
#endif
}

int Fl_Gl_Window::mode(int m, const int *a) {
  if (m == mode_ && a == alist) return 0;
  mode_ = m; alist = a;
  // destroy context and g:
  if (shown()) {destroy(); g = 0; create();}
  return 1;
}

void Fl_Gl_Window::make_current() {
  if (!context) {
#ifdef _WIN32
    context = wglCreateContext(fl_private_dc(this, g));
    if (fl_first_context) wglShareLists(fl_first_context, (GLXContext)context);
    else fl_first_context = (GLXContext)context;
#else
    context = glXCreateContext(fl_display, g->vis, fl_first_context, 1);
    if (!fl_first_context) fl_first_context = (GLXContext)context;
#endif
    valid(0);
  }
  fl_set_gl_context(this, (GLXContext)context);
#if defined(_WIN32) && USE_COLORMAP
  if (fl_palette) {
    fl_window = fl_xid(this); fl_gc = GetDC(fl_window);
    SelectPalette(fl_gc, fl_palette, FALSE);
    RealizePalette(fl_gc);
  }
#endif // USE_COLORMAP
  glDrawBuffer(GL_BACK);
}

void Fl_Gl_Window::ortho() {
// Alpha NT seems to have a broken OpenGL that does not like negative coords:
#ifdef _M_ALPHA
  glLoadIdentity();
  glViewport(0, 0, w(), h());
  glOrtho(0, w(), 0, h(), -1, 1);
#else
  GLint v[2];
  glGetIntegerv(GL_MAX_VIEWPORT_DIMS, v);
  glLoadIdentity();
  glViewport(w()-v[0], h()-v[1], v[0], v[1]);
  glOrtho(w()-v[0], w(), h()-v[1], h(), -1, 1);
#endif
}

void Fl_Gl_Window::swap_buffers() {
#ifdef _WIN32
  SwapBuffers(Fl_X::i(this)->private_dc);
#else
  glXSwapBuffers(fl_display, fl_xid(this));
#endif
}

#if HAVE_GL_OVERLAY && defined(_WIN32)
uchar fl_overlay; // changes how fl_color() works
int fl_overlay_depth = 0;
#endif

void Fl_Gl_Window::flush() {
  uchar save_valid = valid_;

#if HAVE_GL_OVERLAY && defined(_WIN32)
  bool fixcursor = false;
  if (overlay && overlay != this &&
      ((damage()&(FL_DAMAGE_OVERLAY|FL_DAMAGE_ALL|FL_DAMAGE_EXPOSE))
       || !save_valid)) {
    // SGI system messes up overlay over singlebuffer
    fixcursor = !(mode_ & FL_DOUBLE);
    if (fixcursor) SetCursor(0);
    // Draw into hardware overlay planes
    fl_set_gl_context(this, (GLXContext)overlay);
    if (fl_overlay_depth)
      wglRealizeLayerPalette(Fl_X::i(this)->private_dc, 1, TRUE);
    glDisable(GL_SCISSOR_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    fl_overlay = 1;
    draw_overlay();
    fl_overlay = 0;
    valid(save_valid);
    if (damage() == FL_DAMAGE_OVERLAY) {
      wglSwapLayerBuffers(Fl_X::i(this)->private_dc,WGL_SWAP_OVERLAY1);
      if (fixcursor) SetCursor(Fl_X::i(this)->cursor);
      return;
    }
  }
#endif

  make_current();

  if (mode_ & FL_DOUBLE) {

#if SWAP_TYPE == NODAMAGE

    // don't draw if only overlay damage or expose events:
    if ((damage()&~(FL_DAMAGE_OVERLAY|FL_DAMAGE_EXPOSE)) || !save_valid) draw();
    swap_buffers();

#elif SWAP_TYPE == COPY

    // don't draw if only the overlay is damaged:
    if (damage() != FL_DAMAGE_OVERLAY || !save_valid) draw();
    swap_buffers();

#else // SWAP_TYPE == SWAP || SWAP_TYPE == UNDEFINED

    if (overlay == this) { // Use CopyPixels to act like SWAP_TYPE == COPY

      // don't draw if only the overlay is damaged:
      if (damage1_ || damage() != FL_DAMAGE_OVERLAY || !save_valid) draw();
      // we use a seperate context for the copy because rasterpos must be 0
      // and depth test needs to be off:
      static GLXContext ortho_context = 0;
      static Fl_Gl_Window* ortho_window = 0;
      if (!ortho_context) {
#ifdef _WIN32
	ortho_context = wglCreateContext(Fl_X::i(this)->private_dc);
#else
	ortho_context = glXCreateContext(fl_display,g->vis,fl_first_context,1);
#endif
	save_valid = 0;
      }
      fl_set_gl_context(this, ortho_context);
      if (!save_valid || ortho_window != this) {
	ortho_window = this;
	glDisable(GL_DEPTH_TEST);
	glReadBuffer(GL_BACK);
	glDrawBuffer(GL_FRONT);
	glLoadIdentity();
	glViewport(0, 0, w(), h());
	glOrtho(0, w(), 0, h(), -1, 1);
	glRasterPos2i(0,0);
      }
      glCopyPixels(0,0,w(),h(),GL_COLOR);
      make_current(); // set current context back to draw overlay
      damage1_ = 0;

    } else {

#if SWAP_TYPE == SWAP
      uchar old_damage = damage();
      set_damage(damage1_|old_damage); draw();
      swap_buffers();
      damage1_ = old_damage;
#else // SWAP_TYPE == UNDEFINED
      clear_damage(~0); draw();
      swap_buffers();
      damage1_ = ~0;
#endif

    }
#endif

    if (overlay==this) { // fake overlay in front buffer
      glDrawBuffer(GL_FRONT);
      draw_overlay();
      glDrawBuffer(GL_BACK);
      glFlush();
    }

  } else {	// single-buffered context is simpler:

    draw();
    if (overlay == this) draw_overlay();
    glFlush();
#if HAVE_GL_OVERLAY && defined(_WIN32)
    if (fixcursor) SetCursor(Fl_X::i(this)->cursor);
#endif
  }

  valid(1);
}

void Fl_Gl_Window::layout() {
  if (ow() != w() || oh() != h()) valid(0);
  Fl_Window::layout();
}

void Fl_Gl_Window::destroy() {
  if (context) {
    fl_no_gl_context();
    if (context != fl_first_context) {
#ifdef _WIN32
      wglDeleteContext((GLXContext)context);
#else
      glXDestroyContext(fl_display, (GLXContext)context);
#endif
    }
// #ifdef GLX_MESA_release_buffers
//     glXReleaseBuffersMESA(fl_display, fl_xid(this));
// #endif
    context = 0;
  }
#if HAVE_GL_OVERLAY && defined(_WIN32)
  if (overlay && overlay != this && (GLXContext)overlay != fl_first_context) {
    wglDeleteContext((GLXContext)overlay);
    overlay = 0;
  }
#endif
  Fl_Window::destroy();
}

Fl_Gl_Window::~Fl_Gl_Window() {
  destroy();
}

void Fl_Gl_Window::init() {
  end(); // we probably don't want any children
  mode_ = FL_RGB | FL_DEPTH | FL_DOUBLE;
  alist = 0;
  context = 0;
  g = 0;
  overlay = 0;
  damage1_ = 0;
}

void Fl_Gl_Window::draw_overlay() {}

#endif

//
// End of "$Id: Fl_Gl_Window.cxx,v 1.11 2000/05/01 17:25:15 carl Exp $".
//
