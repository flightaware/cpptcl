//#define USE_TCL_STUBS

#ifndef USE_TCL_STUBS
#define CPPTCL_NO_TCL_STUBS
#endif

#include "gd.hpp"

namespace Tcl { namespace GD {
	using namespace detail;
	void init(interpreter * tcli) __attribute__((optimize("s"))) __attribute__((cold));

	void init(interpreter * tcli) {
		auto make_gdio_ctx_from_channel = [] (Tcl_Channel chan) -> gdIOCtx * {
			gdIOCtx * x = new gdIOCtx;
			gdio<Tcl_Channel_IO> * io = new gdio<Tcl_Channel_IO>(chan);
			x->data = (void *) io;
			gdio<Tcl_Channel_IO>::gdio_install(x);
			return x;
		};
		auto make_gdio_ctx_from_name = [&](Tcl_Interp * i, const char * name, int wantmode) -> gdIOCtx * {
			int mode;
			Tcl_Channel chan = Tcl_GetChannel(i, name, &mode);
			if (! chan) return nullptr;
			if (! (mode & wantmode)) return nullptr;
			return make_gdio_ctx_from_channel(chan);
		};

		auto conv_color = [&](Tcl_Obj * o) {
			int ret;
			if (Tcl_GetIntFromObj(tcli->get_interp(), o, &ret) == TCL_OK) {
				return ret;
			}
			int slen;
			const char * s = Tcl_GetStringFromObj(o, &slen);
			auto cmp = [s](const char * b) {
				return strcmp(s, b) == 0;
			};
			if      (cmp("antialiased"))    return gdAntiAliased;
			else if (cmp("brushed"))        return gdBrushed;
			else if (cmp("styled"))         return gdStyled;
			else if (cmp("styled_brushed")) return gdStyledBrushed;
			else if (cmp("tiled"))          return gdTiled;
			else if (cmp("transparent"))    return gdTransparent;
			return 0;
		};

		using I = Tcl::interpreter;

		struct custom {
			static void polygon3(GDImage * img, std::string const & filled_open, list<int> points, int color) {
				gdPoint * pp = new gdPoint[points.size() / 2];
				for (std::size_t i = 0; i < points.size() / 2; ++i) {
					pp[i / 2].x = points.at(i * 2);
					pp[i / 2].y = points.at(i * 2 + 1);
					}
				if (filled_open.empty()) {
					gdImagePolygon(img->img, pp, points.size() / 2, color);
				} else if (filled_open == "filled") {
					gdImageFilledPolygon(img->img, pp, points.size() / 2, color);
				} else if (filled_open == "open") {
					gdImageOpenPolygon(img->img, pp, points.size() / 2, color);
				}
			}
			static void polygon2(GDImage * img, list<int> const & points, int color) {
				polygon3(img, "", points, color);
			}
		};
		auto fromctx = [&] (auto f) {
			return [&, f] (GDFactory *, const char * name) {
				gdIOCtx * ctx = make_gdio_ctx_from_name(tcli->get_interp(), name, TCL_READABLE);
				auto im = f(ctx);
				ctx->gd_free(ctx);
				return named_pointer_result(strcmp(name, "#auto") == 0 ? "" : name, new GDImage { im });
			};
		};
		auto fromdata = [](auto f) {
			return [f] (GDFactory *, const char * name, object const & o) {
				int size;
				unsigned char * ptr = Tcl_GetByteArrayFromObj(o.get_object(), &size);
				return named_pointer_result(strcmp(name, "#auto") == 0 ? "" : name, new GDImage { f(size, ptr) });
			};
		};
		auto fromnew = [](auto f) {
			return [f] (GDFactory *, const char * name, int x, int y) {
				return named_pointer_result(strcmp(name, "#auto") == 0 ? "" : name, new GDImage { f(x, y) });
			};
		};
		
		tcli->class_<GDFactory>("GDFactory", no_init, factory("GDImage"), lambda_wrapper(fromnew), I::cold)
			.def("create",                    gdImageCreate)
			.def("create_truecolor",          gdImageCreateTrueColor);
		
		tcli->class_<GDFactory>("GDFactory", no_init, factory("GDImage"), lambda_wrapper{fromctx}, I::cold)
			.def("create_from_jpeg",          gdImageCreateFromJpegCtx)
			.def("create_from_png",           gdImageCreateFromPngCtx)
			.def("create_from_gif",           gdImageCreateFromGifCtx)
			.def("create_from_gd",            gdImageCreateFromGdCtx)
			.def("create_from_gd2",           gdImageCreateFromGd2Ctx)
			.def("create_from_wbmp",          gdImageCreateFromWBMPCtx);
		
		tcli->class_<GDFactory>("GDFactory", no_init, factory("GDImage"), lambda_wrapper(fromdata), I::cold)
			.def("create_from_jpeg_data",     gdImageCreateFromJpegPtr)
			.def("create_from_png_data",      gdImageCreateFromPngPtr)
			.def("create_from_gif_data",      gdImageCreateFromGifPtr)
			.def("create_from_gd_data",       gdImageCreateFromGdPtr)
			.def("create_from_gd2_data",      gdImageCreateFromGd2Ptr)
			.def("create_from_wbmp_data",     gdImageCreateFromWBMPPtr);
		
		tcli->class_<GDFactory>("GDFactory", no_init, factory("GDImage"), I::cold)
			.def("create_from_gd2_part",      [&](GDFactory *, const char * name, const char * chan, int x, int y, int w, int h) {
					 gdIOCtx * ctx = make_gdio_ctx_from_name(tcli->get_interp(), chan, TCL_READABLE);
					 auto im = gdImageCreateFromGd2PartCtx(ctx, x, y, w, h);
					 ctx->gd_free(ctx);
					 return named_pointer_result(strcmp(name, "#auto") == 0 ? "" : name, new GDImage { im });
				 })
			.def("create_from_gd2_part_data", [&](GDFactory *, const char * name, Tcl_Obj * data, int x, int y, int w, int h) {
					 int sz;
					 unsigned char * ptr = Tcl_GetByteArrayFromObj(data, &sz);
					 auto im = gdImageCreateFromGd2PartPtr(sz, ptr, x, y, w, h);
					 return named_pointer_result(strcmp(name, "#auto") == 0 ? "" : name, new GDImage { im });
				 })
			.def("version", [](GDFactory *) { return object(GD_VERSION_STRING); })
			.def("create_from_xpm", [](GDFactory *, const char * name, const char * fname) {
					 auto im = gdImageCreateFromXpm(const_cast<char *>(fname));
					 return named_pointer_result(strcmp(name, "#auto") == 0 ? "" : name, new GDImage { im });
				 })
			.def("create_from_xbm", [&](GDFactory *, const char * name, const char * fh) {
					 FILE * file;
					 Tcl_GetOpenFile(tcli->get_interp(), fh, 1, 1, (ClientData *) &file);
					 auto im = gdImageCreateFromXbm(file);
					 return named_pointer_result(strcmp(name, "#auto") == 0 ? "" : name, new GDImage { im });
				 });
		//.def("features",...)
		
		tcli->def("GDFactory_construct", [tcli](opt<const char *> const & name) {
					  tcli->custom_construct("GDFactory", name ? *name : "GD", new GDFactory);
				  });
		
		auto conv_imgobj = [](GDImage * this_p, int, Tcl_Obj * const *, int) {
			return this_p->img;
		};
		
#if 1
		tcli->class_<GDImage>("GDImage", no_init)
			.def("pixel",            overload(gdImageGetPixel,  gdImageSetPixel), I::arg<3>, conv_color)
			//.def("pixelrgb",         overload(gdImageGetPixel,  gdImageSetPixel))
			.def("polygon",          overload(custom::polygon2, custom::polygon3));
#endif
			
		// functions with a color argument
		tcli->class_<GDImage>("GDImage", no_init, I::cold, I::arg<-1>, conv_color, I::arg<0>, conv_imgobj)
			.def("line",             gdImageLine)
			.def("rectangle",        gdImageRectangle)
			.def("filled_rectangle", gdImageFilledRectangle)
			.def("arc",              gdImageArc)
			//.def("filled_arc")
			.def("filled_ellipse",   gdImageFilledEllipse)
			.def("fill_to_border",   gdImageFillToBorder, I::arg<3>, conv_color)
			.def("fill",             gdImageFill)
			.def("deallocate_color", gdImageColorDeallocate);
		
		// all else
		tcli->class_<GDImage>("GDImage", no_init, I::cold, I::arg<0>, conv_imgobj)
			.def("allocate_color",              overload(gdImageColorAllocate,  gdImageColorAllocateAlpha))
			.def("closest_color",               overload(gdImageColorClosest,   gdImageColorClosestAlpha))
			.def("closest_color_hwb",           gdImageColorClosestHWB)
			.def("exact_color",                 overload(gdImageColorExact,     gdImageColorExactAlpha))
			.def("resolve_color",               overload(gdImageColorResolve,   gdImageColorResolveAlpha))
			.def("interlace",                   overload([](gdImagePtr img) { return gdImageGetInterlaced(img); },  gdImageInterlace))
			.def("transparent",                 overload([](gdImagePtr img) { return gdImageGetTransparent(img); }, gdImageColorTransparent))
			.def("set_anti_aliased",            gdImageSetAntiAliased)
			.def("set_anti_aliased_dont_blend", gdImageSetAntiAliasedDontBlend, I::arg<-2>, conv_color)
			.def("set_thickness",               gdImageSetThickness)
			.def("alpha_blending",              gdImageAlphaBlending)
			.def("save_alpha",                  gdImageSaveAlpha)
			.def("clip",                        gdImageSetClip)
			.def("bounds_safe",                 gdImageBoundsSafe)
			;
		
		// functions defined as preprocessor macros
		tcli->class_<GDImage>("GDImage", no_init, I::arg<0>, conv_imgobj)
			.def("width",                       [](gdImagePtr img)        { return gdImageSX(img); })
			.def("height",                      [](gdImagePtr img)        { return gdImageSY(img); })
			.def("total_colors",                [](gdImagePtr img)        { return gdImageColorsTotal(img); })
			.def("get_alpha",                   [](gdImagePtr img, int c) { return gdImageAlpha(img, c); })
			.def("green_component",             [](gdImagePtr img, int c) { return gdImageGreen(img, c); })
			.def("blue_component",              [](gdImagePtr img, int c) { return gdImageBlue(img, c); })
			.def("red_component",               [](gdImagePtr img, int c) { return gdImageRed(img, c); })
			.def("true_color",                  overload([](gdImagePtr, int r, int g, int b)        { return gdTrueColor(r, g, b); },
														 [](gdImagePtr, int r, int g, int b, int a) { return gdTrueColorAlpha(r, g, b, a); }))
			;

		auto todata0 = [](auto f) {
			return [f](GDImage * this_p) {
				int sz;
				
				unsigned char * p;
				p = (unsigned char *) f(this_p->img, &sz);
				return object(Tcl_NewByteArrayObj(p, sz));
			};
		};
		auto todata1 = [](auto f) {
			return [f](GDImage * this_p, int i) {
				int sz;
				
				unsigned char * p;
				p = (unsigned char *) f(this_p->img, &sz, i);
				return object(Tcl_NewByteArrayObj(p, sz));
			};
		};
		
		tcli->class_<GDImage>("GDImage", no_init, I::cold)
			.def("png_data",  todata1(gdImagePngPtrEx))
			.def("jpeg_data", todata1(gdImageJpegPtr))
			.def("gif_data",  todata0(gdImageGifPtr))
			.def("wbmp_data", todata1(gdImageWBMPPtr))
			.def("gd_data",   todata0(gdImageGdPtr));
		
		auto toctx_wbmp = [&](auto f) {
			return [&, f](GDImage * img, const char * chan, int i) {
				gdIOCtx * ctx = make_gdio_ctx_from_name(tcli->get_interp(), chan, TCL_WRITABLE);
				f(img->img, i, ctx);
				ctx->gd_free(ctx);
			};
		};
		auto toctx_png_jpeg = [&](auto f) {
			return [&, f](GDImage * img, const char * chan, int i) {
				gdIOCtx * ctx = make_gdio_ctx_from_name(tcli->get_interp(), chan, TCL_WRITABLE);
				f(img->img, ctx, i);
				ctx->gd_free(ctx);
			};
		};
		auto toctx0 = [&](auto f) {
			return [&, f](GDImage * img, const char * chan) {
				gdIOCtx * ctx = make_gdio_ctx_from_name(tcli->get_interp(), chan, TCL_WRITABLE);
				f(img->img, ctx);
				ctx->gd_free(ctx);
			};
		};
		tcli->class_<GDImage>("GDImage", no_init, I::cold)
			.def("write_png",  toctx_png_jpeg(gdImagePngCtxEx))
			.def("write_jpeg", toctx_png_jpeg(gdImageJpegCtx))
			.def("write_gif",  toctx0(gdImageGifCtx))
			.def("write_wbmp", toctx_wbmp(gdImageWBMPCtx))
			;
		
		//.def("write_gd",   toctx(gdImage
	}
} }

#if 1
int main(int argc, char ** argv) {
	using namespace Tcl;
#ifdef USE_TCL_STUBS
	Tcl_Interp * tcl = Tcl_CreateInterpWithStubs("8.6", 0);
#else
	Tcl_Interp * tcl = Tcl_CreateInterp();
#endif
	
	interpreter tcli(tcl, true);
	GD::init(&tcli);
	try {
		auto r = tcli.eval(argv[1]);
		std::cerr << (std::string) r << "\n";
	} catch (std::exception & e) {
		std::cerr << "error: " << e.what() << "\n";
	}
}
#endif
