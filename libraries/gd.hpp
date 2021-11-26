#pragma once

#include <gd.h>

#include <tcl.h>
#include <cpptcl/cpptcl.h>

namespace Tcl {
	namespace GD {
		namespace detail {
			template <typename Impl>
			struct gdio : public Impl {
				typedef Impl this_t;
				using Impl::Impl;
				
				static int getC_(gdIOCtx * x) {
					return ((this_t *) x->data)->getC();
				}
				static int getBuf_(gdIOCtx * x, void * p, int n) {
					return ((this_t *) x->data)->getBuf(p, n);
				}
				static void putC_(gdIOCtx * x, int c) {
					((this_t *) x->data)->putC(c);
				}
				static int putBuf_(gdIOCtx * x, const void * p, int n) {
					return ((this_t *) x->data)->putBuf(p, n);
				}
				static int seek_(gdIOCtx * x, int o) {
					return ((this_t *) x->data)->seek(o);
				}
				static long tell_(gdIOCtx * x) {
					return ((this_t *) x->data)->tell();
				}
				static void free_(gdIOCtx * x) {
					delete (gdio *) x->data;
					delete x;
				}
				static void gdio_install(gdIOCtx * x) {
					x->getC = getC_;
					x->getBuf = getBuf_;
					x->putC = putC_;
					x->putBuf = putBuf_;
					x->tell = tell_;
					x->seek = seek_;
					x->gd_free = free_;
				}
			};
			struct Tcl_Channel_IO {
				Tcl_Channel chan;
				Tcl_Channel_IO(Tcl_Channel c) : chan(c) { }
				int getC() {
					char buf[2];
					Tcl_Read(chan, buf, 1);
					return buf[0];
				}
				void putC(int c) {
					char buf[1];
					buf[0] = (char) c;
					Tcl_Write(chan, buf, 1);
				}
				int getBuf(void * p, int n) {
					return Tcl_Read(chan, (char *) p, n);
				}
				int putBuf(const void * p, int n) {
					return Tcl_Write(chan, (const char *) p, n);
				}
				int seek(int o) {
					return Tcl_Seek(chan, o, SEEK_SET);
				}
				long tell() {
					return Tcl_Tell(chan);
				}
			};

			struct GDFactory {
			};
			struct GDImage : public interpreter::type_ops<GDImage> {
				gdImagePtr img;
				
				GDImage(gdImagePtr i) : img(i) { }
			};
		}
	}
}
