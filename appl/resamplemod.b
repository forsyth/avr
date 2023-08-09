
implement ResampleMod;

include "tk.m";
	tk: Tk;
include "tkclient.m";
	tkclient: Tkclient;
include "sys.m";
	sys : Sys;
include "draw.m";
	draw: Draw;
	Display, Chans, Point, Rect, Image: import draw;
include "./resamplemod.m";

display: ref draw->Display;
context: ref draw->Context;

init(ctxt: ref Draw->Context, vx, vy, px, py: int, img: ref Image, imgpath: string): (ref Image, string)
{
	sys = load Sys Sys->PATH;
	if (sys == nil)
		badmod(Sys->PATH);
	draw = load Draw Draw->PATH;
	if (draw == nil)
		badmod(Draw->PATH);
	tk = load Tk Tk->PATH;
	if (tk == nil)
		badmod(Tk->PATH);
	tkclient = load Tkclient Tkclient->PATH;
	if (tkclient == nil)
		badmod(Tkclient->PATH);
	tkclient->init();
	if (ctxt == nil)
		ctxt = tkclient->makedrawcontext();
	display = ctxt.display;
	context = ctxt;

	if (img == nil) {
		img = display.open(imgpath);
		if (img == nil)
			return (nil, sys->sprint("failed to open image %s: %r", imgpath));
	}
	rgb24img := display.newimage(img.r, Draw->RGB24, 0, Draw->White);
	rgb24img.draw(img.r, img, nil, (0,0));
	origchans := img.chans;
	x,y: int;
	if (px > 0)
		x = (img.r.dx() * px) / 100;
	else
		x = vx;
	if (py > 0)
		y = (img.r.dy() * py) / 100;
	else
		y = vy;

	newr := Rect ((0,0), (x,y));
	if (x == img.r.dx() && y == img.r.dy())
		return (img, nil);
	img = nil;
	newimg := display.newimage(newr, origchans, 0, Draw->White);
	newimg.draw(newr, resample(rgb24img, newr), nil, (0,0));
	return (newimg, nil);
}

resample(img: ref Image, r: Rect): ref Image
{
	newimg := display.newimage(r, Draw->RGB24, 0, Draw->White);

	dx, dy: real;
	dx = real img.r.dx() / real r.dx();
	dy = real img.r.dy() / real r.dy();
	# sys->print("dx: %f dy: %f\n",dx,dy);
	putr := Rect ((0,0),(1,1));
	ry1 := 0.0;
	rx1 := 0.0;
	col := array[3] of real;
	col0 := array[] of {0.0, 0.0, 0.0};
	rx2, ry2: real;
	rdx3 := img.r.dx() * 3;
	for (y := 0; y < r.dy(); y++) {
		ry2 = ry1 + dy;
		wa := array[3 * r.dx()] of byte;
		wai := 0;
		(fy, iy, ly) := makefmap(ry1, ry2);
		getr := Rect ((0,iy),(img.r.dx(), iy + ly));
		a := array[3 * img.r.dx() * ly] of byte;
		img.readpixels(getr,a);

		for (x := 0; x < r.dx(); x++) {
			rx2 = rx1 + dx;
			(fx, ix, lx) := makefmap(rx1, rx2);	
			
			col[:] = col0[:];
			ix3 := ix * 3;

			for (mvy := 0; mvy < ly; mvy++) {
				n := (rdx3 * mvy) + ix3;
				for (mvx := 0; mvx < lx; mvx++) {
					if (n == len a)
						n -= 3;
					f := fx[mvx] * fy[mvy];
					# sys->print("f: %d %d, %f x %f = %f\n",mvx,mvy,fx[mvx],fy[mvy],f);
					for (i := 0; i < 3; i++)
						col[i] += real a[n++] * f;
				}
			}
			writecol := array[3] of byte;
			for (i := 0; i < 3; i++)
				writecol[i] = byte col[i];
			wa[wai:] = writecol[:];
			wai += 3;
			rx1 = rx2;
		}
		newimg.writepixels(((0,y),(r.dx(),y+1)), wa);
		ry1 = ry2;
		rx1 = 0.0;
	}
	return newimg;
}

badmod(path: string)
{
	sys->print("Preview: failed to load: %s\n",path);
	exit;
}

cint2(r: real): int
{
	i := int (r-0.5);
	if (i < 0)
		return 0;
	return i;
}

intup(r: real): int
{
	return int (r + 0.5);
}

makefmap(r1, r2: real): (array of real, int, int)
{
	c2r1 := cint2(r1);
	upr1 := intup(r1);
	downr2 := cint2(r2);
	rupr1 := real upr1;
	rdownr2 := real downr2;
	n := downr2 - upr1;
	if (r1 < rupr1)
		n++;
	intend := 1;
	if (r2 > rdownr2) {
		n++;
		intend = 0;
	}
	a := array[n] of real;
	d := r2 - r1;
	invd := 1.0 / d;
	if (invd > 1.0)
		invd = 1.0;
	for (i := 1; i < n - 1; i++) {
		# sys->print("a[%d] = %f\n",i, 1.0/d);
		a[i] = invd;
	}
	if (!intend) {
		a[n - 1] = (r2 - rdownr2) / d;
		if (a[n - 1] > 1.0)
			a[n - 1] = 1.0;
		# sys->print("a[%d] = %f\n",n-1,a[n-1]);
	}
	else
		a[n - 1] = invd;
	a[0] = ((rupr1) - r1) / d;
	if (a[0] > 1.0)
		a[0] = 1.0;
	# sys->print("a[0] = %f\n",a[0]);
	return (a, c2r1, n);
}
