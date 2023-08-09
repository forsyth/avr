implement Map;

include "tk.m";
	tk: Tk;
include "tkclient.m";
	tkclient: Tkclient;
include "sys.m";
	sys : Sys;
include "daytime.m";
	daytime: Daytime;
include "draw.m";
	draw: Draw;
	Display, Chans, Point, Rect, Image, Font: import draw;
include "readdir.m";
	readdir: Readdir;
include "string.m";
	str: String;
include "bufio.m";
	bufio: Bufio;
	Iobuf: import bufio;
include "rand.m";
	rand: Rand;
include "env.m";
	env: Env;
include "math.m";
	math: Math;
include "sh.m";
	sh: Sh;
include "resamplemod.m";
	resample: ResampleMod;

display: ref draw->Display;
ctxt: ref draw->Context;
ED: con Draw->Enddisc;
MAKETILES: con 0;
sbwidth: int;
FIX, MOVE: con iota;
PLAY, STOP, FWD, RWD, POS, START, END: con iota;
ALL, WEEKDAY, WEEKEND: con iota;
coordlock: chan of ref Tk->Toplevel;
showgridlines := 1;
SELECT, DESELECT, TOGGLE, ZOOM, MVDISP: con iota;
DB, BAT: con iota;
dispmode := DB;
datapath := "/lib/mapdata";

Map: module {
	init: fn (context: ref Draw->Context, argv: list of string);
};

init(context: ref Draw->Context, argv: list of string)
{
	sys = load Sys Sys->PATH;
	if (sys == nil)
		badmod(Sys->PATH);
	draw = load Draw Draw->PATH;
	if (draw == nil)
		badmod(Draw->PATH);
	daytime = load Daytime Daytime->PATH;
	if (daytime == nil)
		badmod(Daytime->PATH);
	tk = load Tk Tk->PATH;
	if (tk == nil)
		badmod(Tk->PATH);
	tkclient = load Tkclient Tkclient->PATH;
	if (tkclient == nil)
		badmod(Tkclient->PATH);
	tkclient->init();
	readdir = load Readdir Readdir->PATH;
	if (readdir == nil)
		badmod(Readdir->PATH);
	resample = load ResampleMod ResampleMod->PATH;
	if (resample == nil)
		badmod(ResampleMod->PATH);
	str = load String String->PATH;
	if (str == nil)
		badmod(String->PATH);
	bufio = load Bufio Bufio->PATH;
	if (bufio == nil)
		badmod(Bufio->PATH);
	rand = load Rand Rand->PATH;
	if (rand == nil)
		badmod(Rand->PATH);
	rand->init(sys->millisec());
	env = load Env Env->PATH;
	if (env == nil)
		badmod(Env->PATH);
	math = load Math Math->PATH;
	if (math == nil)
		badmod(Math->PATH);
	sh = load Sh Sh->PATH;
	if (sh == nil)
		badmod(Sh->PATH);
	sys->pctl(sys->NEWPGRP, nil);
	if (context == nil)
		context = tkclient->makedrawcontext();
	ctxt = context;
	display = ctxt.display;

	if (tl argv != nil)
		excelpath = hd tl argv;

	if (MAKETILES) {
		maketiles("/lib/mapdata/map5000.bit", "mapdata", "map",
			Draw->CMAP8, 5000, 20000, 4, (200, 200), (459330, 452710), 392);
		maketiles("/lib/mapdata/map5000photo.bit", "mapdata", "photo",
			Draw->CMAP8, 5000, 20000, 4, (200, 200), (459330, 452710), 392);
	}

	(top, titlectl) := tkclient->toplevel(ctxt, "", "Map", tkclient->Appl);
	butchan := chan of string;
	tk->namechan(top, butchan, "butchan");

	tkcmds(top, mainscr);
	sbwidth = 4 + int tkcmd(top, ".f.fmap.sy cget -width");

	makelevelimg();
	newcontext: Context;
	C := ref newcontext;
	C.showendx = 0;
	C.showendy = 0;
	C.nselected = 0;
	C.limit = 0;
	C.x = 0;
	C.y = 0;
	C.ext = "map";
	C.path = "/lib/mapdata";
	getmapdata(C);
	C.focusx = real C.max.x / 2.0;
	C.focusy = real C.max.y / 2.0;
	C.playmode = STOP;	
	C.scale = len C.scales - 1;
	C.tkid = nil;
	C.project = nil;
	# loadsensors(C);
	loadlimits(C);
	C.tkid = tkcmd(top, ".f.fmap.c create window 0 0 -window .pmap -anchor nw");

	nhours := 24 * 31;
	(starttime, data) := makeupdata(len C.sensor, nhours);

	C.starttime = starttime;
	C.currtime = starttime;
	C.endtime = starttime + ((len data - 1) * 3600);

	tkcmd(top, "variable playmode "+string C.playmode);
	choices := "{";
	for (i := 0; i < len C.scales; i++)
		choices += "{1:"+string C.scales[i]+"}";
	choices += "}";
	tkcmd(top, ".fscale.cb configure -values "+choices);
	tkcmd(top, "variable scale "+string C.scale);
	tkcmd(top, ". configure -width 500 -height 600");
	
	tkclient->onscreen(top, nil);
	tkclient->startinput(top, "kbd"::"ptr"::nil);
	drawmap(top, C, FIX, nil);

	spawn tkhandler(top);
	projchan := chan of ref Project;
	sensorchan := chan of Sensor;
	limitschan := chan of array of Limit;
	playctl := chan of string;
	timerchan := chan of int;
	spawn timer(playctl, timerchan);
	drag := 0;
	dragp: Point;
	time := daytime->tm2epoch(gettime());
	coordchan: chan of Point = nil;
	coordcc := chan of chan of Point;
	coordlock = chan[1] of ref Tk->Toplevel;
	selectbox := "";
	selectboxp: Point;
	selectbut: int;
	updatedisplay(top, C, data[0]);
	mode := SELECT;
main:
	for(;;) alt {
		coordchan = <-coordcc =>
			sys->print("got new chan: %d\n", coordchan == nil);
		p := <-projchan =>
			C.project = p;
			C.sensor = array[0] of Sensor;
			tkclient->settitle(top, "Map - " + p.name);
			saveproject(C);
			drawmap(top, C, FIX, nil);
		ns := <-sensorchan =>
			found := -1;
			save := 1;
			for (i = 0; i < len C.sensor; i++) {
				if (C.sensor[i].id == ns.id) {
					found = i;
					break;
				}
			}
			if (found == -1) {
				sensor := array[1 + len C.sensor] of Sensor;
				sensor[0:] = C.sensor[:];
				sensor[len sensor - 1] = ns;
				C.sensor = sensor;
				(starttime, data) = makeupdata(len C.sensor, nhours);
			}
			else {
				s := C.sensor[i];
				if (s.loc == ns.loc && s.coord.eq(ns.coord) && s.height == ns.height &&
					s.wall == ns.wall && s.notes == ns.notes)
					save = 0;
				else
					C.sensor[found] = ns;
			}
			if (save) {
				saveproject(C);
				drawmap(top, C, FIX, nil);
			}
		C.limits = <-limitschan =>
			if (C.limit >= len C.limits)
				C.limit = 0;
			savelimits(C);
		inp := <- butchan =>
			(n, lst) := sys->tokenize(inp, " \n\t");
			# sys->print("inp: %s\n",inp);
			case hd lst {
				"sensor" =>
					case hd tl lst {
						"add" =>
							spawn sensorwin(ctxt, -1, C, sensorchan,
								butchan, coordcc);
						"show" =>
							vs := int hd tl tl lst;
							for (i = 0; i < len C.sensor; i++) {
								if (C.sensor[i].selected)
									C.sensor[i].showdisp = vs;
							}
							drawmap(top, C, FIX, nil);
#						"info" =>
#							spawn sensorwin(ctxt, C.selected, C, sensorchan,
#								butchan, coordcc);
						"del" =>
							for (i = 0; i < len C.sensor; i++) {
								if (C.sensor[i].selected) {
									C.sensor[i] = C.sensor[len C.sensor - 1];
									C.sensor = C.sensor[: len C.sensor - 1];
									i--;
								}
							}
							drawmap(top, C, FIX, nil);
#						"mvdisp" =>
#							if (mode == MVDISP) {
#								mode = SELECT;
#								tkcmd(top, ".fsensor.bmv configure -relief raised");
#							}
#							else if (C.selected != -1) {
#								mode = MVDISP;
#								tkcmd(top, ".fsensor.bmv configure -relief flat");
#							}
							tkcmd(top, "update");
							
					}
				"project" =>
					case hd tl lst {
					"new" =>
						spawn projectwin(ctxt, nil, projchan);
					"view" =>
						if (C.project != nil)
							spawn projectwin(ctxt, C.project, projchan);
					"load" =>
						if (tl tl lst == nil)
							spawn loadprojectwin(ctxt, butchan);
						else {
							loadproject(int hd tl tl lst, C);
							tkclient->settitle(top, "Map - "+C.project.name);
							drawmap(top, C, FIX, nil);
							(starttime, data) = makeupdata(len C.sensor, nhours);
						}
					}
				"makebatgraph" =>
					sensors := array[len C.sensor] of int;
					batdata := array[len C.sensor] of int;
					for (i = 0; i < len C.sensor; i++) {
						sensors[i] = i;
						batdata[i] = rand->rand(101);
					}
					spawn graphwin(ctxt, C, BAT, 0, array[] of { batdata },
						sensors, "Sensor Battery Status", batgraphscr);
				"makegraph" =>
					spawn makegraphwin(ctxt, C, data);
				"setlimits" =>
					spawn setlimits(ctxt, C, limitschan);
				"drag" =>
					p := Point (int hd tl lst, int hd tl tl lst);
					if (drag == 0) {
						drag = 1;
						dragp = p;
					}
					else {
						dx := int (real (C.dx * C.scales[0]) / real C.scales[C.scale])/2;
						dy := int (real (C.dy * C.scales[0]) / real C.scales[C.scale])/2;
						pdiff := dragp.sub(p);
						mvx := pdiff.x/dx;
						mvy := pdiff.y/dy;
						setscrollx(C, mvx);
						setscrolly(C, mvy);
						if (mvx != 0 || mvy != 0) {
							drawmap(top, C, MOVE, nil);
							drag = 0;
						}
					}
				"release" =>
					# sys->print("release!\n");
					drag = 0;
					if (selectbox != nil) {
						cl := str->unquoted(tkcmd(top, ".f.fmap.c coords "+selectbox));
						r := Rect ((int hd cl, int hd tl cl), (int hd tl tl cl, int hd tl tl tl cl));
						tkcmd(top, ".f.fmap.c delete "+selectbox+"; update");
						selectbox = nil;
						if (selectbut == 1) {
							if (r.min.eq(r.max)) {
								for (i = 0; i < len C.sensor; i++)
									C.sensor[i].selected = 0;
								s := pix2sensor(C, r.min);
								sys->print("s: %d\n",s);
								if (s != -1)
									C.sensor[s].selected = 1;
								dispsensors(C, C.dispsensorl);
								tkcmd(top, ".pmap dirty");
							}
							else {
								r2 := sortrect((pix2coord(r.min, C),
										pix2coord(r.max, C)));
								sys->print("r2: %s\n", rect2str(r2));
								for (tmp := C.dispsensorl; tmp != nil; tmp = tl tmp) {
									if (C.sensor[hd tmp].coord.in(r2))
										selectsensor(top, C, hd tmp, TOGGLE);
								}
							}
							tkcmd(top, "update");
						}
						else if (selectbut == 3) {
							(x1, y1) := pix2units(r.min, C);
							(x2, y2) := pix2units(r.max, C);
							C.focusx = (x1 + x2) / 2.0;
							C.focusy = (y1 + y2) / 2.0;
														
							nunitsx := real (int (x2 + 0.5) - int (x1 - 0.5));
							nunitsy := real (int (y2 + 0.5) - int (y1 - 0.5));
							w := real tkcmd(top, ".f.fmap.c cget -actwidth");
							h := real tkcmd(top, ".f.fmap.c cget -actheight");
							# sys->print("fx: %f fy: %f\nnux: %f nuy %f\n",
							#	C.focusx, C.focusy,	nunitsx, nunitsy);
							scale := len C.scales - 1;
							for (i = 0; i < len C.scales - 1; i++) {
								dx := real int (real (C.dx * C.scales[0]) / real C.scales[i]);
								dy := real int (real (C.dy * C.scales[0]) / real C.scales[i]);
								sys->print("\tdx: %f w: %f\n\tdy: %f h: %f\n",dx,w,dy,h);
								if (dx * nunitsx <= w && dy * nunitsy <= h) {
									scale = i;
									break;
								} 
							}
							if (scale < C.scale) {
								if (C.fitmap.x)
									C.showendx = 0;
								if (C.fitmap.y)
									C.showendy = 0;
							}
							C.scale = scale;
							zoom(top, C);
						}
					}
				"playmode" =>
					playmode := int hd tl lst;
					if (playmode < POS) {
						C.playmode = int hd tl lst;
						playctl <-= "playmode "+hd tl lst;
					}
					else {
						if (playmode == START)
							C.currtime = C.starttime;
						else if (playmode == END)
							C.currtime = C.endtime;
						else if (playmode == POS) {
							val := int hd tl tl lst;
							if (tl tl tl lst != nil)
								val = int tkcmd(top, ".fplayctl.spos get "+
									hd tl tl lst + " " + hd tl tl tl lst);
							if (val < 0)
								val = 0;
							if (val > 100)
								val = 100;
							tm := daytime->local(C.starttime +
								((C.endtime - C.starttime) * val / 100));
							tm.min = 0;
							tm.sec = 0;
							C.currtime = daytime->tm2epoch(tm);
						}
						updatedisplay(top, C, data[(C.currtime - C.starttime) / 3600]);
					}
				"button3" =>
					pclick := Point (int hd tl lst, int hd tl tl lst);
					if (selectbox == nil) {
						selectbut = 3;
						selectboxp = pclick;
						selectbox = tkcmd(top, ".f.fmap.c create rectangle "+
							rect2str((pclick,pclick))+" -outline black");
					}
					else {
						tkcmd(top, ".f.fmap.c coords "+selectbox+" "+
							rect2str(sortrect((selectboxp, pclick))));
					}
					tkcmd(top, "update");
				"double1" =>
					pclick := Point (int hd tl lst, int hd tl tl lst);
					
					sensor := pix2sensor(C, pclick);
					if (sensor != -1)
						spawn sensorwin(ctxt, sensor, C, sensorchan, butchan, coordcc);

				"button1" =>
					pclick := Point (int hd tl lst, int hd tl tl lst);

					if (coordchan != nil) {
						coordchan <-= pix2coord(pclick, C);
						break;
					}
					if (mode == SELECT) {
						if (selectbox == nil) {
							selectbut = 1;
							selectboxp = pclick;
							selectbox = tkcmd(top, ".f.fmap.c create rectangle "+
								rect2str((pclick,pclick))+" -outline black");
						}
						else {
							tkcmd(top, ".f.fmap.c coords "+selectbox+" "+
								rect2str(sortrect((selectboxp, pclick))));
						}
						tkcmd(top, "update");
					}
					else if (mode == MVDISP) {
#						f := real C.scales[0] / real C.scales[C.scale];
#						sp := coord2pix(C.sensor[C.selected].coord, C);
#						newp := pclick.sub(sp);
#						newp.x = int (real newp.x / f);
#						newp.y = int (real newp.y / f);
#						l := math->sqrt(real ((newp.x * newp.x) + (newp.y * newp.y)));
#						angle := int (rad2deg(math->atan2(real newp.y, real newp.x)));
#						sys->print("angle: %d\n",angle);
#						if (angle >= 0)
#							angle += 8;
#						else
#							angle -= 8;
#						angle = 15 * (angle / 15);
#						
#						newp.x = int (l * math->cos(deg2rad(angle)));
#						newp.y = int (l * math->sin(deg2rad(angle)));
#						C.sensor[C.selected].dispc = newp;
#						drawmap(top, C, FIX, C.selected :: nil);
					}					
				"zoomin" =>
					zoomin(top, C);
				"zoomout" =>
					zoomout(top, C);
				"scale" =>
					scale := int tkcmd(top, "variable scale");
					if (C.scale != scale) {
						C.scale = scale;
						zoom(top, C);
					}
				"scrollx" =>
					if (hd tl lst == "moveto") {
						oldcx := C.x;
						C.x = int (real (C.max.x - 1) * real hd tl tl lst);
						C.showendx = 0;
						sys->print("C.x: %d\n",C.x);
						if (C.x != oldcx)
							drawmap(top, C, MOVE, nil);
					}
					else if (hd tl lst == "scroll") {
						setscrollx(C, int hd tl tl lst);
						sys->print("C.x: %d\n",C.x);
						drawmap(top, C, MOVE, nil);
					}
				"scrolly" =>
					if (hd tl lst == "moveto") {
						oldcy := C.y;
						C.y = int (real (C.max.y - 1) * real hd tl tl lst);
						C.showendy = 0;
						sys->print("C.y: %d\n",C.y);
						if (C.y != oldcy)
							drawmap(top, C, MOVE, nil);
					}
					else if (hd tl lst == "scroll") {
						setscrolly(C, int hd tl tl lst);
						sys->print("C.y: %d\n",C.y);
						drawmap(top, C, MOVE, nil);
					}

			}
		<-timerchan =>
			if (C.playmode == PLAY)
				C.currtime += 3600;
			else if (C.playmode == FWD)
				C.currtime += 2 * 3600;
			else if (C.playmode == RWD)
				C.currtime -= 2 * 3600;
			if (C.currtime < C.starttime) {
				C.currtime = C.starttime;
				C.playmode = STOP;
				tkcmd(top, "variable playmode "+string STOP);
			}
			else if (C.currtime >= C.endtime){
				C.currtime = C.endtime;
				C.playmode = STOP;
				tkcmd(top, "variable playmode "+string STOP);
			}
			updatedisplay(top, C, data[(C.currtime - C.starttime) / 3600]);
		s := <-top.ctxt.ctl or
		s = <-top.wreq or
		s = <- titlectl =>
			if (s == "exit")
				break main;
			tkclient->wmctl(top, s);
			(nil, lst) := sys->tokenize(s, " \t\n");
			if (len lst > 1 && hd lst == "!size" && hd tl lst == ".")
				drawmap(top, C, FIX, nil);
	}
	saveproject(C);
	killg(sys->pctl(0, nil));
}

updatedisplay(top: ref Tk->Toplevel, C: ref Context, sensordata: array of int)
{
	tkcmd(top, ".fplayctl.spos set "+string
		((C.currtime - C.starttime) * 100 / (C.endtime - C.starttime)));
	C.limit = getlimit(C, daytime->local(C.currtime));
	tkcmd(top, ".fplayctl.ltime configure -text {"+formattime(C.currtime, TM2LINE)+
		" "+C.limits[C.limit].desc+"}");
			
	for (i := 0; i < len sensordata; i++) {
		C.sensor[i].val = sensordata[i];
		if (C.sensor[i].val > C.sensor[i].max)
			C.sensor[i].max = C.sensor[i].val;
	}
	dispsensors(C, C.dispsensorl);
	tkcmd(top, ".pmap dirty; update");
}

zoomout(top: ref Tk->Toplevel, C: ref Context)
{
	if (C.scale < len C.scales - 1)
		C.scale++;
	zoom(top, C);
}

zoomin(top: ref Tk->Toplevel, C: ref Context)
{
	if (C.scale > 0)
		C.scale--;
	if (C.fitmap.x)
		C.showendx = 0;
	if (C.fitmap.y)
		C.showendy = 0;
	zoom(top, C);
}

daymatch(daytype, wday: int): int
{
	if (daytype == ALL)
		return 1;
	if (daytype == WEEKDAY && wday > 0 && wday < 6)
		return 1;
	if (daytype == WEEKEND && (wday == 0 || wday == 6))
		return 1;
	return 0;
}

hourmatch(start, end, hour: int): int
{
	if (hour >= start && hour < end)
		return 1;
	else if (end <= start && (hour >= start || hour < end))
		return 1;
	return 0;
}

sortrect(r: Rect): Rect
{
	if (r.min.x > r.max.x) {
		tmp := r.min.x;
		r.min.x = r.max.x;
		r.max.x = tmp;
	}
	if (r.min.y > r.max.y) {
		tmp := r.min.y;
		r.min.y = r.max.y;
		r.max.y = tmp;
	}
	return r;
}

rect2str(r: Rect): string
{
	return sys->sprint("%d %d %d %d", r.min.x, r.min.y, r.max.x, r.max.y);
}

setscrollx(C: ref Context, mv: int)
{
	if (mv < 0 && C.showendx) {
		C.showendx = 0;
		mv++;
	}
	C.x += mv;
	if (C.x < 0)
		C.x = 0;
}

setscrolly(C: ref Context, mv: int)
{
	if (mv < 0 && C.showendy) {
		C.showendy = 0;
		mv++;
	}
	C.y += mv;
	if (C.y < 0)
		C.y = 0;
}

zoom(top: ref Tk->Toplevel, C: ref Context)
{	
	tkcmd(top, "variable scale "+string C.scale);
	if (C.scale == len C.scales - 1)
		tkcmd(top, ".fscale.bzout configure -state disabled");
	else
		tkcmd(top, ".fscale.bzout configure -state normal");
	if (C.scale == 0)
		tkcmd(top, ".fscale.bzin configure -state disabled");
	else
		tkcmd(top, ".fscale.bzin configure -state normal");
	drawmap(top, C, FIX, nil);
}

Context: adt {
	focusx, focusy: real;
	dx,dy, x, y, scale, showendx, showendy, nselected, playmode, limit: int;
	starttime, endtime, currtime: int;
	max, coord, fitmap: Point;
	n500m: int;
	scales: array of int;
	path, ext, tkid: string;
	mapimg: ref Image;
	dispsensorl: list of int;
	sensor: array of Sensor;
	limits: array of Limit;
	project: ref Project;
};

drawmap(top: ref Tk->Toplevel, C: ref Context, focus: int, incl: list of int)
{
#	sys->print("Drawing scale: %d\n",C.scales[C.scale]);

	w := int tkcmd(top, ".f.fmap.c cget -actwidth");
	h := int tkcmd(top, ".f.fmap.c cget -actheight");
	dx := int (real (C.dx * C.scales[0]) / real C.scales[C.scale]);
	dy := int (real (C.dy * C.scales[0]) / real C.scales[C.scale]);
	nx :=  1 + (w / dx);
	ny := 1 + (h / dy);
	if (focus == FIX) {
		rnx := real w / real dx;
		rny := real h / real dy;
		C.x = max(int (C.focusx - (rnx / 2.0)), 0);
		C.y = max(int (C.focusy - (rny / 2.0)), 0);
		sys->print("\nfx: %f nx: %f x: %d\n",C.focusx, rnx, C.x);
		sys->print("fy: %f ny: %f y: %d\n\n",C.focusy, rny, C.y);
	}
	sys->print("x: %d y: %d nx: %d ny: %d endx: %d endy: %d\n",
		C.x, C.y, nx,ny,C.showendx, C.showendy);
	C.fitmap = (0,0);
	if (C.x + nx > C.max.x) {
		C.x = C.max.x - nx;
		if (nx > C.max.x)
			nx = C.max.x;
		if (C.x < 0) {
			C.x = 0;
			C.fitmap.x = 1;
		}
		C.showendx = 1;
	}
	else if (C.x + nx < C.max.x)
		C.showendx = 0;
	if (C.y + ny > C.max.y) {
		C.y = C.max.y - ny;
		if (ny > C.max.y)
			ny = C.max.y;
		if (C.y < 0) {
			C.y = 0;
			C.fitmap.y = 1;
		}
		C.showendy = 1;
	}
	else if (C.y + ny < C.max.y)
		C.showendy = 0;
	if (focus == MOVE) {
		C.focusx = real C.x + (real nx / 2.0);
		C.focusy = real C.y + (real ny / 2.0);
	}
	sys->print("x: %d y: %d nx: %d ny: %d endx: %d endy: %d\n",
		C.x, C.y, nx,ny,C.showendx, C.showendy);

	# sys->print("maxx: %d maxy: %d\n",C.max.x, C.max.y);
	r := Rect ((0,0), (dx*(0+nx), dy*(0+ny)));

	C.mapimg = display.newimage(r, Draw->CMAP8, 0, Draw->White);
#!	mask := display.color(16r999999);
	for (iy := 0; iy < ny; iy++) {
		for (ix := 0; ix < nx; ix++) {
			# sys->print("ix: %d, iy:  %d\n",ix,iy);
			tileimg := display.open(sys->sprint("%s/%d.%d.%d.map",
				 C.path, C.x+ix, C.y+iy, C.scales[C.scale]));
#!			tileimgp := display.open(sys->sprint("%s/%d.%d.%d.photo",
#!				 C.path, C.x+ix, C.y+iy, C.scales[C.scale]));
			if (tileimg != nil) {
				if (showgridlines) {
					points := array[] of {
						tileimg.r.min,
						Point (tileimg.r.min.x, tileimg.r.max.y),
						tileimg.r.max,
						Point (tileimg.r.max.x, tileimg.r.min.y),
						tileimg.r.min,
					};
					tileimg.poly(points , ED, ED, 0, display.color(16r00000033), (0,0));
				}

				C.mapimg.draw(((ix*dx, iy*dy), ((ix+1)*dx, (iy+1)*dy)), tileimg, nil, (0,0));
#!				C.mapimg.draw(((ix*dx, iy*dy), ((ix+1)*dx, (iy+1)*dy)), tileimgp, nil, (0,0));
#!				C.mapimg.draw(((ix*dx, iy*dy), ((ix+1)*dx, (iy+1)*dy)), tileimg, mask, (0,0));
			}
		}
	}
	tkcmd(top, ".f.fmap.c configure -scrollregion {0 0 "+
		string C.mapimg.r.dx() + " " + string C.mapimg.r.dy() + "}");
	totalx := real (C.max.x * dx);
	totaly := real (C.max.y * dy);
	startx := real C.x / real C.max.x;
	starty := real C.y / real C.max.y;
	endx := minr(startx + (real w/totalx), 1.0);
	endy := minr(starty + (real h/totaly), 1.0);
	if (C.showendx) {
		tkcmd(top, ".f.fmap.c xview moveto 1");
		startx += 1.0 - endx;
		endx = 1.0;
	}
	else
		tkcmd(top, ".f.fmap.c xview moveto 0");

	if (C.showendy){
		tkcmd(top, ".f.fmap.c yview moveto 1");
		starty += 1.0 - endy;
		endy = 1.0;
	}
	else
		tkcmd(top, ".f.fmap.c yview moveto 0");
	sys->print("startx: %f endx: %f\n",startx,endx);
	tkcmd(top, ".f.fmap.sx set "+string startx+" "+string endx);
	tkcmd(top, ".f.fmap.sy set "+string starty+" "+string endy);

	dispr := Rect (pix2coord((0,C.mapimg.r.max.y), C),
				pix2coord((C.mapimg.r.max.x, C.mapimg.r.min.y), C));
	C.dispsensorl = incl;
	for (i := 0; i < len C.sensor; i++) {
		if (C.sensor[i].coord.in(dispr)) {
#			C.mapimg.fillellipse(coord2pix(sensorcoord[i], C), 
#				2,2,display.rgb(255,0,0), (0,0));
			C.dispsensorl = i :: C.dispsensorl;
		}
	}
	dispsensors(C, C.dispsensorl);

#	sys->print("dispr: (%d, %d) (%d, %d)\n", dispr.min.x, dispr.min.y, dispr.max.x, dispr.max.y);

	tk->putimage(top, ".pmap", C.mapimg, nil);
	tkcmd(top, ".pmap dirty; update");
}

dispsensors(C: ref Context, lsens: list of int)
{
	rd := 2;
	f := real C.scales[0] / real C.scales[C.scale];
	for (tmp := lsens; tmp != nil; tmp = tl tmp) {
		sensor := C.sensor[hd tmp];
		psensor := coord2pix(sensor.coord, C);
		pdisp := Point (int (real sensor.dispc.x * f), int (real sensor.dispc.y * f));
		if (sensor.showdisp)
			C.mapimg.line(psensor, psensor.add((pdisp.x, pdisp.y)),
					ED, ED, 0, display.black, (0,0));
		if (sensor.selected)
			C.mapimg.fillellipse(psensor, rd,rd,display.rgb(0,255,0), (0,0));
		else
			C.mapimg.fillellipse(psensor, rd,rd,display.rgb(255,0,0), (0,0));
		C.mapimg.ellipse(psensor, 2,2,0,display.black, (0,0));
		
		if (sensor.showdisp) {
			pdisp.y -= int (real 10 * f);
			drawlevel(C.mapimg, psensor.add(pdisp), sensor.val, C.limits[C.limit].maxdb, f);
		}
	}
}

barwidth := 10;
maxdb: con 80;
mindb: con 0;

drawlevel(img: ref Image, p: Point, level, max: int, f: real)
{
	w := int (real barwidth * f);
	p = p.sub((w/2,0));
	h:= int (40.0 * f);
	l := int (real (level * 40) / real maxdb * f);
	m := int (real (max * 40) / real maxdb * f);
	polyp := array[] of {
		p,
		p.add((0, 1+h)),
		p.add((w, 1+h)),
		p.add((w, 0)),
		p,
	};
#	img.fillpoly(polyp, 0, display.white, (0,0));
	img.fillpoly(polyp, 0, display.black, (0,0));
	img.poly(polyp, ED, ED, 0, display.black, (0,0));
	
#	for (i := 1; i <= l; i++) {
#		row := int (real i / f);
#		img.draw((p.add((1, h+1-i)), p.add((w, h+2-i))), levelimg, nil, (0,40 - row));
#	}
	if (dispmode == DB) {
		img.draw((p.add((1, h+1-l)), p.add((w, h+1))), display.rgb(0,255,0), nil, (0,0));
		if (max > 0) {
			if (level >= max)
				img.draw((p.add((1,h+1-l)),p.add((w, h+1-m))),
					display.rgb(255,0,0), nil,(0,0));
			img.draw((p.add((0, h+1-m)), p.add((w+1, h+2-m))), display.white, nil, (0,0));
		}
	}
	else {
		if (max > 0)
			img.draw((p.add((1, h+1-l)), p.add((w, h+1))), display.rgb(255,0,0), nil, (0,0));
	}
}

levelimg: ref Image;

makelevelimg()
{
	levelimg = display.newimage(((0,0),(barwidth, 40)), Draw->CMAP8, 0, Draw->Black);
	for (i := 0; i < 20; i++)
		levelimg.draw(((0,i),(barwidth, i+1)), display.rgb(255, (255 * i) / 19, 0), nil, (0,0));
	for (i = 0; i < 20; i++)
		levelimg.draw(((0,i+20),(barwidth, i+21)), 
		display.rgb(255 - ((255 * i) / 19), 255, 0), nil, (0,0));
}

badmod(path: string)
{
	sys->print("Map: failed to load: %s\n",path);
	exit;
}

tkcmd(top: ref Tk->Toplevel, cmd: string): string
{
	e := tk->cmd(top, cmd);
	if (e != "" && e[0] == '!') sys->print("tk error: '%s': %s\n",cmd,e);
	return e;
}

coord2pix(p: Point, C: ref Context): Point
{
	f := real C.scales[0] / real C.scales[C.scale];
	p.x -= C.coord.x;
	p.y = -(p.y - C.coord.y);
	pix2m := real C.n500m * f;
	p.x = int (real p.x / 500.0 * pix2m);
	p.y = int (real p.y / 500.0 * pix2m);
	p.x -= int (real (C.x * C.dx) * f);
	p.y -= int (real (C.y * C.dy) * f);
	return p;
}

pix2coord(p: Point, C: ref Context): Point
{
	f := real C.scales[0] / real C.scales[C.scale];
	p.x += int (real (C.x * C.dx) * f);
	p.y += int (real (C.y * C.dy) * f);
	pix2m := real C.n500m * f;
	p.x = int (real (p.x * 500) / pix2m);
	p.y = int (real (p.y * 500) / pix2m);
	p.x += C.coord.x;
	p.y = C.coord.y - p.y;
	return p;
}

pix2units(p: Point, C: ref Context): (real, real)
{
	dx := real (C.dx * C.scales[0]) / real C.scales[C.scale];
	dy := real (C.dy * C.scales[0]) / real C.scales[C.scale];
	
	return (real C.x + (real p.x / dx), real C.y + (real p.y / dy));
}

tkcmds(top: ref Tk->Toplevel, cmds: array of string)
{
	for (i := 0; i < len cmds; i++)
		tkcmd(top, cmds[i]);
}

getmapdata(C: ref Context)
{
	fd := sys->open(C.path+"/data."+C.ext, sys->OREAD);
	if (fd != nil) {
		buf := array[sys->ATOMICIO] of byte;
		i := sys->read(fd, buf, len buf);
		datain := str->unquoted(string buf[:i]);
		if (len datain >= 5) {
			C.max = (int hd datain, int hd tl datain);
			datain = tl tl datain;
			C.coord = (int hd datain, int hd tl datain);
			datain = tl tl datain;
			C.n500m = int hd datain;
			datain = tl datain;

			C.scales = array[len datain] of int;
			i = len C.scales;
			for (; datain != nil; datain = tl datain)
				C.scales[--i] = int hd datain;
		}
		img := display.open(sys->sprint("%s/0.0.%d.%s", C.path, C.scales[0], C.ext));
		if (img != nil) {
			C.dx = img.r.dx();
			C.dy = img.r.dy();
		}
	}
}

maketiles(path, outpath, ext: string, chans: Draw->Chans, scale, minscale, steps: int, tile, coord: Point, n500m: int)
{
	startscale := scale;
	ldimg := display.open(path);
	if (ldimg == nil) {
		sys->fprint(sys->fildes(2), "could not open image: %r\n");
		return;
	}
	scales: list of string = nil;
	maxx, maxy: int;
	for (j := 0; j < steps; j++) {
		scale = startscale + (j * ((minscale - startscale)/(steps-1)));
		scales = string scale :: scales;
		sf := real startscale / real scale;
		vx := int (real ldimg.r.dx() * sf);
		vy := int (real ldimg.r.dy() * sf);
		tx := int (real tile.x * sf);
		ty:= int (real tile.y * sf);
		sys->print("resampling %s: %d\n",ext, scale);
		(rsimg, err) := resample->init(ctxt, vx, vy, -1, -1, ldimg, nil);
		if (ldimg == nil || err != nil) {
			sys->fprint(sys->fildes(2), "Map: Error: %s: %r\n", err);
			return;
		}
		sys->print("writing files...");
		maxy = rsimg.r.dy()/ty;
		maxx = rsimg.r.dx()/tx;
		for (y := 0; y <= maxy; y++) {
			for (x := 0; x <= maxx; x++) {
				r := Rect ((x * tx, y*ty),((x+1)*tx, (y+1)*ty));
				newimg := display.newimage(((0,0),(r.dx(), r.dy())), chans, 0, Draw->White);
				newimg.draw(newimg.r, rsimg, nil, r.min);
				fname := sys->sprint("%d.%d.%d.%s", x,y, scale, ext);
				fd := sys->create(outpath+"/"+fname, sys->OWRITE, 8r666);
				if (fd == nil)
					sys->print("Could not create %s: %r\n",fname);
				else
					display.writeimage(fd, newimg);
				# sys->print("map: (%d, %d)\n",r.dx(), r.dy());
				# ldimg.poly(ps, ED, ED, 0, display.rgb(255,0,0), (0,0));
			}
		}
		sys->print("done\n");
	}
	fd := sys->create(outpath+"/data."+ext, sys->OWRITE, 8r666);
	if (fd != nil)
		sys->fprint(fd, "%d %d %d %d %d %s", 1+maxx, 1+maxy,
			coord.x, coord.y, n500m, str->quoted(scales));
}

max(a, b: int): int
{ 
	if (a > b)
		return a;
	return b;
}

minr(a, b: real): real
{ 
	if (a < b)
		return a;
	return b;
}

abs(a: int): int
{
	if (a < 0)
		return -a;
	return a;
}

font: con " -font /fonts/charon/plain.normal.font ";
fontb: con " -font /fonts/charon/bold.normal.font ";

mainscr := array[] of {
	"frame .f",

	"frame .f.fmenu",
	"menubutton .f.fmenu.fproj -text {Project} -menu .mproj"+font,
	"menubutton .f.fmenu.fopt -text {Options} -menu .mopt"+font,
	"menu .mproj"+font,
	".mproj add command -label {View details} -command {send butchan project view}",
	".mproj add separator",
	".mproj add command -label {New} -command {send butchan project new}",
	".mproj add command -label {Open} -command {send butchan project load}",

	"menu .mopt"+font,
	".mopt add command -label {Add new sensor} -command {send butchan sensor add}",
	".mopt add command -label {Set noise limits} -command {send butchan setlimits}",
	".mopt add command -label {Create graph} -command {send butchan makegraph}",
	".mopt add command -label {Show battery life} -command {send butchan makebatgraph}",
	"grid .f.fmenu.fproj .f.fmenu.fopt -row 0 -column 0",

	"frame .f.fmap",
	"scrollbar .f.fmap.sx -orient horizontal -command {send butchan scrollx}",
	"scrollbar .f.fmap.sy -command {send butchan scrolly}",
	"canvas .f.fmap.c -bg white -height 50 -width 50",
	"panel .pmap",
	"bind .pmap <Button-1> {send butchan button1 %x %y}",
	"bind .pmap <Double-Button-1> {send butchan double1 %x %y}",
	"bind .pmap <Button-2> {send butchan drag %x %y}",
	"bind .pmap <Button-3> {send butchan button3 %x %y}",
	"bind .pmap <ButtonRelease> {send butchan release}",
	"frame .f.fctl -borderwidth 2 -relief raised",
	"frame .fscale",
	"choicebutton .fscale.cb -variable scale -command {send butchan scale} -bg white"+font,
	"button .fscale.bzin -text {+} -command {send butchan zoomin} -takefocus 0"+font,
	"button .fscale.bzout -text {-} -command {send butchan zoomout}" +
		" -takefocus 0 -width 19 -state disabled"+font,

	"frame .fsensor -borderwidth 1 -relief raised",
#	"label .fsensor.l -text {Sensor}"+fontb,
	"button .fsensor.bdel -text {X} -width 20 -takefocus 0 -command {send butchan sensor del} -fg red"+font,
	"button .fsensor.binfo -text {I} -width 20 -takefocus 0 -command {send butchan sensor info}"+font,
	"button .fsensor.bmv -text {m} -width 20 -takefocus 0 -command {send butchan sensor mvdisp}"+font,

	"button .fsensor.bshow -text {Show} -takefocus 0 "+
		"-command {send butchan sensor show 1}"+font,
	"button .fsensor.bhide -text {Hide} -takefocus 0 "+
		"-command {send butchan sensor show 0}"+font,

#	"grid .fsensor.l .fsensor.binfo .fsensor.bmv .fsensor.bdel .fsensor.bshow .fsensor.bhide -row 0 -sticky w",
	"grid .fsensor.bdel .fsensor.bshow .fsensor.bhide -row 0 -sticky w",

	"frame .fplayctl -borderwidth 2 -relief raised",
	"radiobutton .fplayctl.bplay -indicatoron 0 -takefocus 0 -borderwidth 2 -relief raised"+
		" -selectcolor green -text {>} -width 30 -variable playmode -value "+string PLAY+
		" -command {send butchan playmode "+string PLAY+"}"+font,
	"radiobutton .fplayctl.bstop -indicatoron 0 -takefocus 0 -borderwidth 2 -relief raised"+
		" -selectcolor red -text {l l} -width 30 -variable playmode -value "+string STOP+
		" -command {send butchan playmode "+string STOP+"}"+font,
	"radiobutton .fplayctl.bfwd -indicatoron 0 -takefocus 0 -borderwidth 2 -relief raised"+
		" -selectcolor blue -text {>>} -width 30 -variable playmode -value "+string FWD+
		" -command {send butchan playmode "+string FWD+"}"+font,
	"radiobutton .fplayctl.brwd -indicatoron 0 -takefocus 0 -borderwidth 2 -relief raised"+
		" -selectcolor blue -text {<<} -width 30 -variable playmode -value "+string RWD+
		" -command {send butchan playmode "+string RWD+"}"+font,
	"scale .fplayctl.spos -from 0 -to 100 -orient horizontal -showvalue 0"+
		" -height 16 -width 130 -command {send butchan playmode "+string POS+"}",
	"bind .fplayctl.spos <Button-2> {send butchan playmode "+string POS+" %x %y}",
	"button .fplayctl.bstart -font /fonts/charon/plain.tiny.font -text {l<} -takefocus 0"+
		" -command {send butchan playmode "+string START+"} -height 10 -width 14",
	"button .fplayctl.bend -font /fonts/charon/plain.tiny.font -text {>l} -takefocus 0"+
		" -command {send butchan playmode "+string END+"} -height 10 -width 14",

	"frame .fplayctl.fpos",
	"grid .fplayctl.bstart .fplayctl.spos .fplayctl.bend -in .fplayctl.fpos -row 0",
	"label .fplayctl.ltime -text {\n } -bg black -borderwidth 2 -relief sunken"+
		" -fg white -width 150 -anchor w -height 45"+font,
	"grid .fplayctl.brwd .fplayctl.bstop .fplayctl.bplay .fplayctl.bfwd -row 0 -padx 5 -pady 5",
	"grid .fplayctl.ltime -row 0 -column 4 -rowspan 2 -sticky snw -padx 5 -pady 5",
	"grid .fplayctl.fpos -row 1 -column 0 -columnspan 4 -sticky w -padx 5 -pady 5",

	"grid .fscale.bzin -row 0 -column 0",
	"grid .fscale.cb -row 0 -column 1 -padx 5",
	"grid .fscale.bzout -row 0 -column 2",
	
	"grid .fplayctl -in .f.fctl -row 0 -padx 10 -pady 10 -columnspan 3",
	"grid .fscale -in .f.fctl -row 1 -padx 10 -pady 10",
	"grid .fsensor -in .f.fctl -row 1 -column 2 -padx 10 -pady 10",

	"pack .f -fill both -expand 1",
	"grid .f.fmap.sy -row 0 -column 0 -sticky ns",
	"grid .f.fmap.c -row 0 -column 1 -sticky nsew",
	"grid .f.fmap.sx -row 1 -column 1 -sticky ew",
	"grid rowconfigure .f.fmap 0 -weight 1",
	"grid columnconfigure .f.fmap 1 -weight 1",

	"pack .f.fmenu -anchor w -side top",
	"pack .f.fmap -fill both -expand 1 -side top",
	"pack .f.fctl -fill x -side top",
	"pack propagate . 0; update",

};

tkhandler(top: ref Tk->Toplevel)
{
	for (;;) alt {
		s := <-top.ctxt.kbd =>
			tk->keyboard(top, s);
		s := <-top.ctxt.ptr =>
			tk->pointer(top, *s);
	}
}

Sensor: adt {
	id: int;
	loc, notes: string;
	coord, dispc: Point;
	val, max, height, wall, showdisp, selected: int;
};

#loadsensors(C: ref Context)
#{
#	ID, LOC, NOTES, CX, CY, DX, DY, HEIGHT, WALL: con iota;
#	iobuf := bufio->open(C.path+"/sensors", bufio->OREAD);
#	if (iobuf == nil) {
#		sys->print("Error: could not open %s/sensors: %r\n",C.path);
#		return;
#	}
#	lsensor: list of Sensor = nil;
#	for (;;) {
#		s := iobuf.gets('\n');
#		if (s == nil)
#			break;
#		lst := str->unquoted(s);
#		if (len lst >= 5) {
#			a := list2array(lst);
#			lsensor = Sensor (int a[ID], a[LOC], a[NOTES], (int a[CX], int a[CY]),
#						(int a[DX], int a[DY]), 0, 0, int a[HEIGHT],
#						int a[WALL], 0, 0) :: lsensor;
#		}
#	}
#	C.sensor = array[len lsensor] of Sensor;
#	i := len C.sensor;
#	for (; lsensor != nil; lsensor = tl lsensor)
#		C.sensor[--i] = hd lsensor;
#}

#savesensors(C: ref Context)
#{
#	fd := sys->create(C.path+"/sensors", sys->OWRITE, 8r666);
#	if (fd == nil) {
#		sys->print("Error: could not open %s/sensors: %r\n",C.path);
#		return;
#	}
#	for (i := 0; i < len C.sensor; i++) {
#		s := C.sensor[i];
#		ls := string s.id :: s.loc :: s.notes :: string s.coord.x :: string s.coord.y ::
#			string s.dispc.x :: string s.dispc.y :: string s.height :: string s.wall :: nil;
#		sys->fprint(fd, "%s\n", str->quoted(ls));
#	}
#}

loadlimits(C: ref Context)
{
	DESC, DAY, START, END, MAXDB: con iota;
	iobuf := bufio->open(C.path+"/limits", bufio->OREAD);
	if (iobuf == nil) {
		sys->print("Error: could not open %s/limits: %r\n",C.path);
		return;
	}
	llimit: list of Limit = nil;
	for (;;) {
		s := iobuf.gets('\n');
		if (s == nil)
			break;
		lst := str->unquoted(s);
		if (len lst >= 5) {
			a := list2array(lst);
			llimit = Limit (a[DESC], int a[DAY], int a[START],
					int a[END], int a[MAXDB]) :: llimit;
		}
	}
	C.limits = array[1 + len llimit] of Limit;
	i := len C.limits;
	for (; llimit != nil; llimit = tl llimit)
		C.limits[--i] = hd llimit;
	C.limits[0] = Limit("Unspecified", ALL, 0, 0, 0);
}

savelimits(C: ref Context)
{
	sys->print("Save limits\n");
	fd := sys->create(C.path+"/limits", sys->OWRITE, 8r666);
	if (fd == nil) {
		sys->print("Error: could not open %s/limits: %r\n",C.path);
		return;
	}
	for (i := 1; i < len C.limits; i++) {
		s := C.limits[i].desc :: string C.limits[i].day :: string C.limits[i].start ::
			string C.limits[i].end :: string C.limits[i].maxdb :: nil;
		sys->fprint(fd, "%s\n", str->quoted(s));
	}
}

list2array[T](l: list of T): array of T
{
	a := array[len l] of T;	
	for (i := 0; l != nil; l = tl l)
		a[i++] = hd l;
	return a;
}

timer(ctl: chan of string, chanout: chan of int)
{
	mode := STOP;
	wait := 1000;
	n := 0;
	for (;;) alt {
		inp := <-ctl =>
			(nil, lst) := sys->tokenize(inp, " \t\n");
			case hd lst {
				"playmode" =>
					mode = int hd tl lst;
					if (mode == FWD || mode == RWD)
						wait = 500;
					else
						wait = 1000;
					n = wait;
				"delay" =>
					wait = int hd tl lst;
			}
		* =>
			if (mode != STOP) {
				n += 500;
				if (n >= wait) {
					n = 0;
					chanout <-= 1;
				}
			}
			sys->sleep(500);
	}
}

kill(pid: int)
{
	if ((fd := sys->open("/prog/" + string pid + "/ctl", Sys->OWRITE)) != nil)
		sys->fprint(fd, "kill");
}

killg(pid: int)
{
	if ((fd := sys->open("/prog/" + string pid + "/ctl", Sys->OWRITE)) != nil)
		sys->fprint(fd, "killgrp");
}

LENTIME: con 1;
TM2LINE: con 2;
TM1LINE: con 4;
TMSHORT: con 8;

timepart(secs, tlen: int, tdesc, s: string): (int, string)
{
	if (secs == 0)
		return (0, s);
	if (secs >= tlen) {
		v := secs;
		secs = secs % tlen;
		v = (v - secs) / tlen;
		if (v != 0) {
			s += ", " + string v + " " + tdesc;
			if (v != 1)
				s[len s] = 's';
		}
	}
	return (secs, s);
}

formattime(t: int, mode: int): string
{
	if (mode == LENTIME) {
		s := "";
		(t, s) = timepart(t, WEEK, "week", s);
		(t, s) = timepart(t, DAY, "day", s);
		(t, s) = timepart(t, HOUR, "hour", s);
		(t, s) = timepart(t, MIN, "min", s);
		if (s == nil)
			s = "  0 hrs";
		return s[2:];
	}
	else {
		tm := daytime->local(t);
		lst := str->unquoted(daytime->text(tm));
		day := hd lst;
		mon := hd tl lst;
		date := hd tl tl lst;
		time := hd tl tl tl lst;
		time = time[: len time - 3];
		year := hd tl tl tl tl tl lst;
		if (mode == TM2LINE)
			return sys->sprint("%s %s %s %s\n%s", day, date, mon, year, time);
		else if (mode & TMSHORT) {
			val := mode >> 4;
			if (val == 0)
				return sys->sprint("%s %s %s %s", day, date, mon, year);
			else if (val == 1)
				return sys->sprint("%s %s %s", date, mon, year);
			else
				return sys->sprint("%s/%d/%s", date, tm.mon, year);
		}
		else
			return sys->sprint("%s %s %s %s %s", day, date, mon, year, time);
	}
}

Limit: adt {
	desc: string;
	day, start, end, maxdb: int;
};

cm2m(cm: int): string
{
	rcm := cm % 100;
	m := (cm - rcm) / 100;
	return sys->sprint("%d.%dm", m, rcm);
}

gettime(): ref daytime->Tm
{
	tm := daytime->local(daytime->now() - (60 * 60 * 24 * 7));
	tm.sec = 0;
	tm.min = 0;
	tm.hour = 6;
	return tm;
}

limitsscr := array[] of {
	"frame .f",
	"scrollbar .f.s -command {.f.c yview}",
	"canvas .f.c -yscrollcommand {.f.s set} -height 150",
	"frame .flimits",
	".f.c create window 0 0 -window .flimits -anchor nw",
	"button .f.bnew -text {New} -command {send butchan new} -takefocus 0"+font,
	"button .f.bok -text { Ok } -command {send butchan ok} -takefocus 0"+font,
	"button .f.bcancel -text {Cancel} -command {send butchan cancel} -takefocus 0"+font,
	"label .f.ldesc -text {Description} -anchor e -width 114"+font,
	"label .f.lday -text { } -width 80 -anchor w"+font,
	"label .f.lfrom -text {From} -width 52 -anchor w"+font,
	"label .f.lto -text {To} -width 52 -anchor w"+font,
	"label .f.llimit -text {Limit (dB)}"+font,
	
	"frame .f.row1",
	"frame .f.row2",
	"frame .f.row3",
	"pack .f.ldesc .f.lday .f.lfrom .f.lto .f.llimit -side left -in .f.row1",
	"pack .f.row1 -side top -anchor w",
	"pack .f.row2 -side top -fill both -expand 1",
	"pack .f.row3 -side top -fill x",
	"pack .f.s -in .f.row2 -side left -fill y",
	"pack .f.c -in .f.row2 -side left -fill both -expand 1",
	"grid .f.bnew .f.bok .f.bcancel -in .f.row3 -row 0 -padx 10 -pady 10",
	"pack .f -fill both -expand 1",
	"pack propagate . 0; update",
};

setlimits(ctxt: ref Draw->Context, C: ref Context, chanout: chan of array of Limit)
{
	(top, titlectl) := tkclient->toplevel(ctxt, "", "Set Noise Limits", tkclient->Appl);
	butchan := chan of string;
	tk->namechan(top, butchan, "butchan");
	tkcmds(top, limitsscr);
	for (i := 1; i < len C.limits; i++) {
		makelimitstk(top, i);
		f := ".flimit"+string i;
		tkcmd(top, f+".edesc insert 0 {"+C.limits[i].desc+"}");
		tkcmd(top, f+".bday set "+string C.limits[i].day);
		tkcmd(top, f+".estart insert 0 {"+string C.limits[i].start+"}");
		tkcmd(top, f+".eend insert 0 {"+string C.limits[i].end+"}");
		tkcmd(top, f+".elimit insert 0 {"+string C.limits[i].maxdb+"}");
	}
	nlimits := len C.limits;
	if (nlimits == 0) {
		makelimitstk(top, nlimits);
		tkcmd(top, ".flimits0.estart insert 0 {hh}");
		tkcmd(top, ".flimits0.eend insert 0 {hh}");
		nlimits++;
	}
		
	tkcmd(top, ".f.c configure -scrollregion {0 0 0 "+tkcmd(top, ".flimits cget -height")+"}");
	tkcmd(top, "update");
	tkclient->onscreen(top, nil);
	tkclient->startinput(top, "kbd"::"ptr"::nil);
	for (;;) alt {
		s := <-top.ctxt.kbd =>
			tk->keyboard(top, s);
		s := <-top.ctxt.ptr =>
			tk->pointer(top, *s);
		inp := <-butchan =>
			(nil, lst) := sys->tokenize(inp, " \t\n");
			case hd lst {
			"cancel" =>
				return;
			"ok" =>
				lst2 := str->unquoted(tkcmd(top, "grid slaves .flimits -column 0"));
				limits := array[1 + len lst2] of Limit;
				limits[0] = C.limits[0];
				failed: list of string = nil;
				modified := len limits != len C.limits;
				for (i = 1; lst2 != nil; lst2 = tl lst2) {
					desc := getentry(top, hd lst2+".edesc");
					if (desc == nil)
						failed = hd lst2+".edesc" :: failed;
					limits[i].desc = desc;
					start := int getentry(top, hd lst2+".estart");
					if (start < 0 || start > 23)
						failed = hd lst2+".estart" :: failed;
					limits[i].start = start;
					end := int getentry(top, hd lst2+".eend");
					if (end < 0 || end > 23)
						failed = hd lst2+".eend" :: failed;
					limits[i].end = end;
					dblimit := int getentry(top, hd lst2+".elimit");
					if (dblimit < 0 || dblimit > 150)
						failed = hd lst2+".elimit" :: failed;
					limits[i].maxdb = dblimit;
					day := int tkcmd(top, hd lst2+".bday get");
					limits[i].day = day;
					if (!modified) {
						if (C.limits[i].desc != desc || C.limits[i].start != start ||
							C.limits[i].end != end || C.limits[i].day != day ||
							C.limits[i].maxdb != dblimit)
							modified = 1;
					}
					i++;
				}
				if (failed == nil) {
					if (len limits == 0)
						limits = array[] of { Limit ("Default", ALL, 0, 0, 55), };
					if (modified)
						chanout <-= limits;
					return;
				}
				for (; failed != nil; failed = tl failed) {
					tkcmd(top, hd failed+" configure -bg yellow");
					if (tl failed == nil)
						tkcmd(top, "focus "+hd failed);
				}			
			"key" =>
				name := hd tl tl lst;
				sn := hd tl lst;
				widget := ".flimit" + sn + ".e" + name;
				s := " ";
				s[0] = int hd tl tl tl lst;
				if (s == "\\")
					s = "\\\\";
				case s {
				"\t" or "\n" =>
					tkcmd(top, widget+" selection clear");
					if (name == "desc")
						tkcmd(top, "focus .flimit"+sn+".estart");
					else if (name == "start")
						tkcmd(top, "focus .flimit"+sn+".eend");
					else if (name == "end")
						tkcmd(top, "focus .flimit"+sn+".elimit");
					else {
						lst2 := str->unquoted(tkcmd(top, 
							"grid slaves .flimits -column 0"));
						first := hd lst2;
						for (; lst2 != nil; lst2 = tl lst2) {
							if (hd lst2 == ".flimit"+sn) {
								if (tl lst2 == nil)
									tkcmd(top, "focus "+first+".edesc");
								else
									tkcmd(top, "focus "+hd tl lst2+".edesc");
								break;
							}
						}
					}
				* =>
					if (name == "desc" || (s >= "0" && s <= "9")) {
						if (tkcmd(top, widget + " selection present") == "1")
							tkcmd(top, widget + " delete sel.first sel.last");
						tkcmd(top, widget + " insert insert {"+s+"}");
						tkcmd(top, widget + " see insert");
					}
				}
			"new" =>
				f := ".flimit"+string nlimits;
				makelimitstk(top, nlimits++);
				tkcmd(top, f+".estart insert 0 {hh}");
				tkcmd(top, f+".eend insert 0 {hh}");
				tkcmd(top, "focus "+f+".edesc");
				tkcmd(top, ".f.c configure -scrollregion {0 0 0 "+
					tkcmd(top, ".flimits cget -height")+"}");
			"del" =>
				tkcmd(top, "destroy .flimit"+hd tl lst);
				tkcmd(top, ".f.c configure -scrollregion {0 0 0 "+
					tkcmd(top, ".flimits cget -height")+"}");
			}
			tkcmd(top, "update");
		s := <-top.ctxt.ctl or
		s = <-top.wreq or
		s = <- titlectl =>
			if (s == "exit")
				return;
			tkclient->wmctl(top, s);
	}
}

makelimitstk(top: ref Tk->Toplevel, n: int)
{
	sn := string n;
	f := ".flimit"+sn;
	tkcmd(top, "frame "+f);
	tkcmd(top, "button "+f+".bdel -text {X} -fg red -activeforeground red"+
		" -command {send butchan del "+sn+"} -takefocus 0"+fontb);
	tkcmd(top, "entry "+f+".edesc -bg white -width 80 -borderwidth 1"+font);
	tkcmd(top, "choicebutton "+f+".bday -values {{Every day} {Weekdays} {Weekends}}"+font);

	tkcmd(top, "entry "+f+".estart -bg white -width 50 -borderwidth 1"+font);
	tkcmd(top, "entry "+f+".eend -bg white -width 50 -borderwidth 1"+font);
	tkcmd(top, "entry "+f+".elimit -bg white -width 50 -borderwidth 1"+font);
	tkcmd(top, "bind  "+f+".edesc <Key> {send butchan key "+sn+" desc %s}");
	tkcmd(top, "bind "+f+".estart <Key> {send butchan key "+sn+" start %s}");
	tkcmd(top, "bind "+f+".eend <Key> {send butchan key "+sn+" end %s}");
	tkcmd(top, "bind "+f+".elimit <Key> {send butchan key "+sn+" limit %s}");

	tkcmd(top, "grid "+f+".bdel -row 0 -column 0");
	tkcmd(top, "grid "+f+".edesc -row 0 -column 1 -sticky w");
	tkcmd(top, "grid "+f+".bday -row 0 -column 2 -sticky w");
	tkcmd(top, "grid "+f+".estart -row 0 -column 3 -sticky w");
	tkcmd(top, "grid "+f+".eend -row 0 -column 4 -sticky w");
	tkcmd(top, "grid "+f+".elimit -row 0 -column 5 -sticky w");
	tkcmd(top, "grid "+f+" -in .flimits -row "+sn);
}

sensorscr := array[] of {
	"frame .f",

	"button .f.bok -text { Ok } -command {send butchan ok} -takefocus 0"+font,
	"button .f.bcancel -text {Cancel} -command {send butchan cancel} -takefocus 0"+font,
	
	"label .f.lid -text {Id: }"+font,
	"label .f.lname -text {Desc: }"+font,
	"label .f.lpos -text {Posn: }"+font,
	"label .f.lheight -text {Height: }"+font,
	"label .f.lnotes -text {Notes: }"+font,
	"label .f.lc -text {,}"+font,
	"label .f.lm -text {metres}"+font,
	"entry .f.eid -width 50 -bg white"+font,
	"entry .f.ename -width 204 -bg white"+font,
	"entry .f.epx -width 80 -bg white"+font,
	"entry .f.epy -width 80 -bg white"+font,
	"entry .f.eheight -width 50 -bg white"+font,
	"checkbutton .f.bwall -variable wall -text {Sensor is <1m from a wall}"+font,
	"text .f.tnotes -bg white -wrap word -yscrollcommand {.f.s set}"+font,
	"scrollbar .f.s -command {.f.tnotes yview}",

	"bind .f.eid <Key> {send butchan key id %s}",
	"bind .f.ename <Key> {send butchan key name %s}",
	"bind .f.epx <Key> {send butchan key px %s}",
	"bind .f.epy <Key> {send butchan key py %s}",
	"bind .f.eheight <Key> {send butchan key height %s}",
	"bind .f.tnotes <Control-t> {send butchan key notes TAB}",
	"bind .f.tnotes {<Key-	>} {send butchan key notes %s}",
	"bind .f.bwall {<Key-	>} {send butchan walltab}",

	"frame .f.fdesc",
	"frame .f.fpos",
	"frame .f.fheight",
	"frame .f.fnotes",
	"frame .f.fbut",
	"grid .f.epx .f.lc .f.epy -in .f.fpos -row 0",
	"grid .f.bok .f.bcancel -in .f.fbut -row 0 -padx 10 -pady 10",
	"grid .f.eheight .f.lm -in .f.fheight -row 0 -sticky w",
	"grid .f.tnotes .f.s -in .f.fnotes -row 0 -sticky nsw",
	"grid .f.eid .f.lname .f.ename -in .f.fdesc -row 0 -sticky w",

	"grid .f.lid -row 0 -column 0 -sticky w",
#	"grid .f.lname -row 1 -column 0 -sticky w",
	"grid .f.lpos -row 2 -column 0 -sticky w",
	"grid .f.lheight -row 3 -column 0 -sticky w",
	"grid .f.lnotes -row 4 -column 0 -sticky w",

#	"grid .f.eid -row 0 -column 1 -sticky w",
	"grid .f.fdesc -row 0 -column 1 -columnspan 2 -sticky w",
#	"grid .f.ename -row 1 -column 1 -columnspan 2 -sticky w",
	"grid .f.fpos -row 2 -column 1 -sticky w -columnspan 2",
	"grid .f.fheight -row 3 -column 1 -sticky w",
	"grid .f.bwall -row 3 -column 2 -sticky w",
	"grid .f.fnotes -row 4 -column 1 -columnspan 2 -sticky w",
	"grid .f.fbut -row 5 -column 0 -columnspan 3",
	"pack .f; update",
};

sensorwin(ctxt: ref Draw->Context, sensorid: int, C: ref Context, sensorchan: chan of Sensor, closechan: chan of string, chanout: chan of chan of Point)
{
	(top, titlectl) := tkclient->toplevel(ctxt, "", "Add Sensor", tkclient->Popup);
	alt {
		coordlock <-= top =>;
		* =>
			oldtop := <-coordlock;
			coordlock <-= oldtop;
			tkclient->wmctl(oldtop, "kbdfocus 1");
			return;
	}
	coordchan := chan of Point;
	chanout <-= coordchan;
	butchan := chan of string;

	wlist := array[] of {"eid", "ename", "epx", "epy", "eheight", "bwall", "tnotes", "eid"};

	tk->namechan(top, butchan, "butchan");
	tkcmds(top, sensorscr);
	if (sensorid != -1) {
		tkcmd(top, ".f.eid insert end {" + string C.sensor[sensorid].id + "}");
		tkcmd(top, ".f.eid configure -state disabled");
		tkcmd(top, ".f.ename insert end {" + string C.sensor[sensorid].loc + "}");
		tkcmd(top, ".f.epx insert end {" + string C.sensor[sensorid].coord.x + "}");
		tkcmd(top, ".f.epy insert end {" + string C.sensor[sensorid].coord.y + "}");
		tkcmd(top, ".f.eheight insert end {" + cm2m(C.sensor[sensorid].height) + "}");
		tkcmd(top, "variable wall " + string C.sensor[sensorid].wall);
		tkcmd(top, ".f.tnotes insert 1.0 '" + C.sensor[sensorid].notes);
	}	
	else
		tkcmd(top, "focus .f.eid");
	tkclient->onscreen(top, nil);
	tkclient->startinput(top, "kbd"::"ptr"::nil);
main:
	for (;;) alt {
		s := <-top.ctxt.kbd =>
			tk->keyboard(top, s);
		s := <-top.ctxt.ptr =>
			tk->pointer(top, *s);
		p := <-coordchan =>
			tkcmd(top, "raise .; focus .f.epx;");
			tkcmd(top, ".f.epx delete 0 end");
			tkcmd(top, ".f.epx insert 0 {"+string p.x+"}");
			tkcmd(top, ".f.epy delete 0 end");
			tkcmd(top, ".f.epy insert 0 {"+string p.y+"}");
			tkcmd(top, "update");
			tkclient->wmctl(top, "kbdfocus 1");
		inp := <-butchan =>
			(nil, lst) := sys->tokenize(inp, " \t\n");
			case hd lst {
			"cancel" =>
				break main;
			"ok" =>
				failed: list of string = nil;
				for (i := 0; i < len wlist - 3; i++) {
					w := ".f."+wlist[i];
					if (getentry(top, w) == nil)
						failed = w :: failed;
				}
				s: Sensor;
				s.id = int tkcmd(top, ".f.eid get");
				if (sensorid == -1) {
					for (i = 0; i < len C.sensor; i++)
						if (C.sensor[i].id == s.id)
							failed = ".f.eid" :: failed;
					s.dispc = (25, 0);
					s.showdisp = 0;
					s.selected = 0;
				}
				else {
					s.dispc = C.sensor[sensorid].dispc;
					s.showdisp = C.sensor[sensorid].showdisp;				
					s.selected = C.sensor[sensorid].selected;
				}
				s.loc = tkcmd(top, ".f.ename get");
				s.coord = (int tkcmd(top, ".f.epx get"), int tkcmd(top, ".f.epy get"));
				s.height = int (100.0 * real tkcmd(top, ".f.eheight get"));
				s.notes = tkcmd(top, ".f.tnotes get 1.0 end");
				s.wall = int tkcmd(top, "variable wall");
				s.val = 0;
				s.max = 0;
				if (failed != nil) {
					for (; failed != nil; failed = tl failed) {
						tkcmd(top, hd failed+" configure -bg yellow");
						if (tl failed == nil)
							tkcmd(top, "focus "+hd failed);
					}
				}
				else {
					sensorchan <-= s;
					break main;
				}
			"walltab" =>
				tkcmd(top, "focus .f.tnotes");
			"key" =>
				name := hd tl lst;
				widget := ".f.e" + name;
				s := " ";
				s[0] = int hd tl tl lst;
				if (s == "\\")
					s = "\\\\";
				# sys->print("inp: %s\n",inp);
				if (name == "notes") {
					if (s == "\t") {
						if (sensorid == -1)
							tkcmd(top, "focus .f.eid");
						else
							tkcmd(top, "focus .f.ename");
					}
					else {
						if (hd tl tl lst == "TAB")
							s = "\t";
						if (tkcmd(top, ".f.tnotes tag ranges sel") != "")
							tkcmd(top, ".f.tnotes delete sel.first sel.last");
						tkcmd(top, ".f.tnotes insert insert {"+s+"}; .f.tnotes see insert");
					} 
				}
				else case s {
				"\t" or "\n" =>
					tkcmd(top, widget+" selection clear");
					for (i := 0; i < len wlist; i++) {
						if (name == wlist[i][1:]) {
							tkcmd(top, "focus .f."+wlist[i+1]);
							break;
						}
					}
				* =>
					if (name == "name" ||
						((name[0] == 'p' || name == "id") && s >= "0" && s <= "9") ||
						(name == "height" && ((s >= "0" && s <= "9") || s == "."))) {
						if (tkcmd(top, widget + " selection present") == "1")
							tkcmd(top, widget + " delete sel.first sel.last");
						tkcmd(top, widget + " insert insert {"+s+"}");
					}
				}
			}
			tkcmd(top, "update");
		s := <-top.ctxt.ctl or
		s = <-top.wreq or
		s = <- titlectl =>
			if (s == "exit")
				break main;
			tkclient->wmctl(top, s);
	}
	chanout <-= nil;
	<-coordlock;
	closechan <-= "closeaddsensor";
}

Gap: adt {
	w, n, e, s: int;
};

batgraphscr := array[] of {
	"frame .f -bg white",
	"canvas .f.c -bg white -height 50 -width 10 -borderwidth 0 -yscrollcommand {.f.s set}",
	"panel .f.p",
	"scrollbar .f.s -command {.f.c yview}",
	".f.c create window 0 0 -window .f.p -anchor nw",

	"pack .f -fill both -expand 1",
	"grid .f.s -row 0 -column 0 -sticky ns",
	"grid .f.c -row 0 -column 1 -sticky nsew",
	"grid rowconfigure .f 0 -weight 1",
	"grid columnconfigure .f 1 -weight 1",
};

drawbatlife(top: ref Tk->Toplevel, data, sensors: array of int, labels: array of string)
{
	size := Point (int tkcmd(top, ".f.c cget -actwidth"), int tkcmd(top, ".f.c cget -actheight"));
	nsensors := len sensors;
	drawfont := Font.open(display, "/fonts/charon/plain.small.font");

	gap := Gap (0, 10, 20, 30);
	pps := (size.y - gap.n - gap.s) / nsensors;
	if (pps < drawfont.height + 5)
		pps = drawfont.height + 5;

	tkcmd(top, "label .l -font /fonts/charon/plain.normal.font");
	w := 0;
	for (i := 0; i < len labels; i++) {
		tkcmd(top, ".l configure -text {"+labels[i]+"}");
		w = max(w, int tkcmd(top, ".l cget -width"));
	}
	tkcmd(top, "destroy .l");

	size.y = gap.n + gap.s + (pps * nsensors);
	size.x = max(size.x, gap.e + gap.w + w + 200);
	graphimg := display.newimage(((0,0), size), Draw->CMAP8, 0, Draw->White);

	pppc := real (size.x - gap.e - gap.w - w) / 100.0;
	npc := 1;
	lmul := 5 :: 10 :: 20 :: 25 :: 50 :: 100 :: nil;
	while (lmul != nil && real npc * pppc < 5.0) {
		npc = hd lmul;
		lmul = tl lmul;
	}	
	nnpc := npc;
	while (lmul != nil && real nnpc * pppc < 20.0) {
		nnpc = hd lmul;
		lmul = tl lmul;
	}	

	# Y-Axis
	for (i = 0; i < nsensors; i++) {
		y := gap.n + i * pps;
		graphimg.line((gap.w + w - 5, y), (gap.w + w, y), ED, ED, 0, display.black, (0,0));
		p := graphimg.text((0, y+1), display.white, (0,0), drawfont, labels[i]);
		graphimg.text((gap.w + w - p.x - 3, y+(pps - drawfont.height)/2),
			display.black, (0,0), drawfont, labels[i]);
		graphimg.draw(((gap.w + w, y + 1), (gap.w + w + int (pppc * real data[i]), y + pps - 1)),
			display.rgb(255,0,0), nil, (0,0));
	}

	# X-Axis
	for (i = npc; i <= 100; i += npc) {
		x := gap.w + w + int (real i * pppc);
		graphimg.line((x,size.y - gap.s), (x, size.y - gap.s + 5),
			ED, ED, 0, display.black, (0,0));
		if (i % nnpc == 0 && i > 0) {
			graphimg.line((x,size.y - gap.s), (x, gap.n),
				ED, ED, 0, display.color(16r73737380), (0,0));
#				ED, ED, 0, display.color(16r00000020), (0,0));
			graphimg.line((x,size.y - gap.s), (x, size.y - gap.s + 7),
				ED, ED, 0, display.black, (0,0));
			val := string i;
			graphimg.text((x - 4 * len val, size.y - gap.s + 10),
				display.black, (0,0), drawfont, val);
		}
	}

	# Draw axis
	graphimg.line((w + gap.w, gap.n), (w + gap.w, size.y - gap.s), ED, ED, 0, display.black, (0,0));
	graphimg.line((w + gap.w, size.y - gap.s), (size.x - gap.e, size.y - gap.s),
		ED, ED, 0, display.black, (0,0));
	graphimg.text((size.x - gap.e + 5, size.y - gap.s - drawfont.height/2),
			display.black, (0,0), drawfont, "%");
#	graphimg.line((w + gap.w, gap.n), (size.x - gap.e, gap.n),
#		ED, ED, 0, display.color(16r00000020), (0,0));

	tk->putimage(top, ".f.p", graphimg, nil);
	tkcmd(top, ".f.c configure -scrollregion {0 0 "+string graphimg.r.dx() +
		" " + string graphimg.r.dy() + "}");
	tkcmd(top, ".f.p dirty; update");
}

drawgraph(top: ref Tk->Toplevel, C: ref Context, starttime: int, data: array of array of int, sensors: array of int, labels: array of string)
{
	size := Point (int tkcmd(top, ".f.c cget -actwidth"), int tkcmd(top, ".f.c cget -actheight"));
	nsensors := len sensors;
	lastp := array[nsensors] of Point;

	# Draw Colours
	colimg := array[nsensors] of ref Image;
	colr := ((0,0),(2,2));
	for (i := 0; i < nsensors; i++) {
		colimg[i] = display.newimage(colr, Draw->RGB24, 1, Draw->White);
		colimg[i].draw(colr, display.color(graphcol[i % len graphcol]), nil, (0,0));
		if (i >= len graphcol)
			colimg[i].line((0,0), (1,1), ED, ED, 0, display.white, (0,0));
	}
	# Draw key
	tk->cmd(top, "destroy .fkey");
	tkcmd(top, "frame .fkey -bg white");
	cr := Rect ((0,0),(6,6));
	for (i = 0; i < nsensors; i++) {
		si := string i;
		tkcmd(top, "panel .fkey.p"+si+" -borderwidth 1 -bg black -relief flat");
		tkcmd(top, "label .fkey.l"+si+" -bg white -text {"+labels[i]+"}"+font);
		tkcmd(top, "grid .fkey.p"+si+" -row "+si+" -column 0 -padx 5 -pady 2 -sticky w");
		tkcmd(top, "grid .fkey.l"+si+" -row "+si+" -column 1 -pady 2 -sticky w");
		col := display.newimage(cr, Draw->RGB24, 0, Draw->White);
		col.draw(cr, colimg[i], nil, (0,0));
		tk->putimage(top, ".fkey.p"+si, col, nil);
	}
	tkcmd(top, "button .fkey.bxl -text {Send to Excel} -takefocus 0 -command {send butchan excel} -font /fonts/charon/plain.small.font");
	tkcmd(top, "grid .fkey.bxl -pady 20 -row "+string (1+nsensors)+" -column 0 -columnspan 2");
	w := tkcmd(top, ".fkey cget -width");
	h := tkcmd(top, ".fkey cget -height");
	tkcmd(top, ".f.fkey.c create window 0 0 -window .fkey -anchor nw");
	tkcmd(top, ".f.fkey.c configure -scrollregion {0 0 "+ w + " " + h + "}");
	if (int h > (size.y - 16))
		tk->cmd(top, "pack .f.fkey.sy -side left -fill y");
	else
		tk->cmd(top, "pack forget .f.fkey.sy");
	size = Point (int tkcmd(top, ".f.c cget -actwidth"), int tkcmd(top, ".f.c cget -actheight"));

	nhours := len data - 1;
	drawfont := Font.open(display, "/fonts/charon/plain.small.font");
	graphimg := display.newimage(((0,0), size), Draw->RGB24, 0, Draw->White);
	gap := Gap (30, 30, 40, 40);
	endtime := starttime + (HOUR * len data);

	titlefont := Font.open(display, "/fonts/charon/plain.normal.font");
	v := 0;
	for (;;) {
		title := formattime(starttime, TMSHORT | v << 4) + " - " +
			formattime(endtime, TMSHORT | v << 4);
		titlelen := graphimg.text((0, 0), display.white, (0,0), titlefont, title);
		if (titlelen.x <= size.x - gap.w - gap.e) {
			graphimg.text(((size.x - titlelen.x) / 2, (gap.n - titlefont.height) / 2),
				display.black, (0,0), titlefont, title);
			sys->print("title: %d %d\n", size.x + (titlelen.x / 2), (gap.n - titlelen.y) / 2);
			break;
		}
		if (v == 2) {
			size.x = titlelen.x + gap.w + gap.e;
			graphimg = display.newimage(((0,0), size), Draw->RGB24, 0, Draw->White);
		}
		v++;
	}	

	# Draw axis
	graphimg.line((gap.w, gap.n), (gap.w, size.y - gap.s), ED, ED, 0, display.black, (0,0));
	graphimg.line((gap.w, size.y - gap.s), (size.x - gap.e, size.y - gap.s),
		ED, ED, 0, display.black, (0,0));
	graphimg.text((size.x - gap.e + 5, size.y - gap.s + 5), display.black, (0,0), drawfont, "Time");
	graphimg.text((size.x - gap.e + 5, size.y - gap.s + 19),
		display.rgb(150,150,150), (0,0), drawfont, "Date");

	# Draw DB scale (Y-Axis)
	graphimg.text((gap.w - 16, 10), display.black, (0,0), drawfont, "dB");
	ppdb := real (size.y - gap.n - gap.s) / real (maxdb - mindb);
	ndb := 5;
	if (real ndb * ppdb < 10.0)
		ndb = 10;
	dist := 0;
	ppmark := int (real ndb * ppdb);
	lastmarked := mindb;
	sys->print("ndb: %d ppdb: %f\n",ndb,ppdb);
	for (i = mindb; i <= maxdb; i+= ndb) {
		y := size.y - gap.s - int (real i * ppdb);
		graphimg.line((gap.w - 5, y), (gap.w, y), ED, ED, 0, display.black, (0,0));
		if (dist >= 20 && maxdb % (i - lastmarked) == 0) {
			val := string i;
			graphimg.text((gap.w - 5 - 8 * len val, y - 6),
				display.black, (0,0), drawfont, val);
			graphimg.line((1+gap.w, y), (size.x - gap.e, y),
				ED, ED, 0, display.rgb(230,230,230), (0,0));
			dist = 0;
			lastmarked = i;
		}
		dist += ppmark;
	}

	# Draw time scale (X-Axis)
	pph := real (size.x - gap.w - gap.e) / real nhours;
	hrs := 1;
	lmul := 2 :: 4 :: 6 :: 8 :: 12 :: 24 :: 48 :: nil;
	while (lmul != nil && real hrs * pph < 5.0) {
		hrs = hd lmul;
		lmul = tl lmul;
	}
	
	tm := daytime->local(starttime);
	hour := tm.hour;
	diff := (24 - hour) % hrs;
	# sys->print("hour: %d hrs: %d diff: %d\n",hour, hrs, diff);
	addhr := int (real hrs * pph);
	ppday := int (real 24 * pph);
	n := 0;
	for (i = 0; i < hour; i++) {
		if (i % hrs == 0) {
			if (n >= 20)
				n = 0;
			n += addhr;
		}
	}
	newtm: ref Daytime->Tm;
	for (i = 0; i <= nhours; i++) {
		if (hour == 0 || i == 0) {
			x := gap.w + int (real i * pph);
			newtm = daytime->local(starttime + (60*60*i));
			if (newtm.wday == 0 || newtm.wday == 6) {
				x2 := x + int (real (24 - hour) * pph);
				if (x2 > size.x - gap.e)
					x2 = size.x - gap.e;
				graphimg.draw(((x, size.y - gap.s + 1),
					(1 + x2, size.y - gap.s + 33)),
					display.rgb(230,230,230), nil, (0,0));
			}
		}
		if (hour == 0) {
			x := gap.w + int (real i * pph);
			graphimg.line((x, size.y - gap.s), (x, size.y - gap.s + 32),
				ED, ED, 0, display.rgb(150,150,150), (0,0));
			if (i != 0)
				graphimg.line((x, size.y - gap.s - 1), (x, gap.n),
					ED, ED, 0, display.rgb(230,230,230), (0,0));
			if (ppday > 20 && i != nhours) {
				val := string newtm.mday;
				if (ppday > 50)
					val += "/"+ string (newtm.mon + 1);
				graphimg.text((x + 4, size.y - gap.s + 19),
					display.rgb(150,150,150), (0,0), drawfont, val);
			}
			n = 0;
		}
		if ((i-diff) % hrs == 0) {
			x := gap.w + int (real i * pph);
			graphimg.line((x, size.y - gap.s), (x, size.y - gap.s + 5),
				ED, ED, 0, display.black, (0,0));
			if (n >= 20 && ppday > 40) {
				if (hour != 0 && i != 0) {
					graphimg.line((x, size.y - gap.s), (x, gap.n),
						ED, ED, 0, display.rgb(230,230,230), (0,0));
					graphimg.line((x, size.y - gap.s), (x, size.y - gap.s + 7),
						ED, ED, 0, display.black, (0,0));
					val := string hour;
					graphimg.text((1 + x - 4 * len val, size.y - gap.s + 7),
						display.black, (0,0), drawfont, val);
				}
				n = 0;
			}
			n += addhr;	
		}
		hour = (1+hour) % 24;	
	}

	# draw limits	
	hour = 0;
	done := 0;
	x1 := 1 + gap.w;
	while (!done) {
		newtm = daytime->local(starttime + (3600 * hour));
		limit := getlimit(C, newtm);
		# sys->print("%s: Limit: %d\n",daytime->text(newtm), C.limits[limit].maxdb);
		if (limit == 0) {
			hour++;
			x1 = 1 + gap.w + int (real hour * pph);
		}
		else {
			limlen := C.limits[limit].end - C.limits[limit].start;
			if (hour == 0)
				limlen -= newtm.hour - C.limits[limit].start;
			if (limlen < 0)
				limlen = C.limits[limit].end + (24 - C.limits[limit].start);
			else if (limlen == 0 && limit != 0)
				limlen = 24;
			x2 := 1 + gap.w + int (real (hour + limlen) * pph);
			if (x2 > 1+size.x - gap.e) {
				x2 = 1+size.x - gap.e;
				done = 1;
			}
			y := size.y - gap.s - int (real C.limits[limit].maxdb * ppdb);
			graphimg.draw(((x1, y), (x2, size.y - gap.s)),
				display.black, display.color(16r00000010), (0,0));
			hour += limlen;
			x1 = x2;
		}
	}

	# Draw graphs
	for (i = 0; i <= nhours; i++) {
		x := gap.w + int (real i * pph);
		for (s := 0; s < nsensors; s++) {
			if (data[i][sensors[s]] == -1)
				lastp[s] = (-1,-1);
			else {
				p := Point(x, size.y - gap.s - int (real data[i][sensors[s]] * ppdb));
				if (lastp[s].x == -1)
					lastp[s] = p;
				if (i > 0)
					graphimg.line(lastp[s], p, ED, ED, 0, colimg[s], (0,0));
				lastp[s] = p;
			}
		}
	}

	tk->putimage(top, ".f.p", graphimg, nil);
	tkcmd(top, ".f.p dirty; update");
}

makegraphscr := array[] of {
	"frame .f",
	"frame .f.fbut",
	"button .f.fbut.bn -text {Next >>} -command {send butchan next}" +
		" -takefocus 0 -state disabled" + font,
	"button .f.fbut.bb -text {<< Back} -command {send butchan back} -takefocus 0" + font,
	"button .f.fbut.bc -text {Cancel} -command {send butchan cancel} -takefocus 0" + font,
	"button .f.fbut.bd -text {Draw Graph} -command {send butchan draw} -takefocus 0" + font,
	"grid .f.fbut.bn -row 0 -column 1 -pady 10",
	"grid .f.fbut.bc -row 0 -column 2 -padx 20 -pady 10",
	"grid .f.fbut -row 1 -column 0 -sticky e",
	"grid rowconfigure .f 0 -weight 1",
	"grid columnconfigure .f 0 -weight 1",
	"pack .f -fill both -expand 1",

	"frame .ftime",
	"frame .ftime.fdisp -borderwidth 2 -relief sunken",
	"frame .ftime.fdisp.f1 -width 0",
	"frame .ftime.fdisp.f2 -bg red -width 0",
	"frame .ftime.fdisp.f3 -width 0",
	"label .ftime.fdisp.lstart -relief raised -borderwidth 2 -width 5 -height 20",
	"label .ftime.fdisp.lend -relief raised -borderwidth 2 -width 5 -height 20",
	"bind .ftime.fdisp.lstart <Button-1> {send butchan set start %X}",
	"bind .ftime.fdisp.lend <Button-1> {send butchan set end %X}",
	"grid .ftime.fdisp.f1 -sticky nsew -row 0 -column 0",
	"grid .ftime.fdisp.lstart -row 0 -column 1",
	"grid .ftime.fdisp.f2 -sticky nsew -row 0 -column 2",
	"grid .ftime.fdisp.lend -row 0 -column 3",
	"grid .ftime.fdisp.f3 -sticky nsew -row 0 -column 4",
	"grid columnconfigure .ftime.fdisp 2 -weight 1",
	"grid columnconfigure .ftime 0 -weight 1",
	
	"label .ftime.lstartl -text {Start time:}"+font,
 	"label .ftime.lstart -text {}"+font,
 	"label .ftime.lendl -text {End time:}"+font,
 	"label .ftime.lend -text {}"+font,
 	"label .ftime.llenl -text {Length:}"+font,
 	"label .ftime.llen -text {}"+font,

 	"label .ftime.l -text {Step 2: Select time period} -height 30 -anchor w"+fontb,
	"frame .ftime.fls",
	"grid .ftime.lstartl -in .ftime.fls -row 0 -column 0 -sticky w -pady 2",
	"grid .ftime.lstart -in .ftime.fls -row 0 -column 1 -sticky w -pady 2",
	"grid .ftime.lendl -in .ftime.fls -row 1 -column 0 -sticky w -pady 2",
	"grid .ftime.lend -in .ftime.fls -row 1 -column 1 -sticky w -pady 2",
	"grid .ftime.llenl -in .ftime.fls -row 2 -column 0 -sticky w -pady 2",
	"grid .ftime.llen -in .ftime.fls -row 2 -column 1 -sticky w -pady 2",

	"grid .ftime.l -row 0 -column 0 -sticky ew",
	"grid .ftime.fdisp -row 1 -column 0 -sticky nsew -padx 20",
	"grid .ftime.fls -row 2 -column 0 -sticky w -padx 5 -pady 20",
	
	"frame .fsensor",
	"listbox .fsensor.l1 -bg white -selectbackground blue -selectmode multiple " +
		"-xscrollcommand {.fsensor.sx1 set} -yscrollcommand {.fsensor.sy1 set}" +
		" -selectforeground white -takefocus 0 -height 30 -width 30" + font,
	"scrollbar .fsensor.sx1 -orient horizontal -command {.fsensor.l1 xview}",
	"scrollbar .fsensor.sy1 -command {.fsensor.l1 yview}",
	"button .fsensor.bin -text { > } -command {send butchan move 1 2} -takefocus 0"+font,
	"button .fsensor.bout -text { < } -command {send butchan move 2 1} -takefocus 0"+font,
	"listbox .fsensor.l2 -bg white -selectbackground blue -selectmode multiple " +
		"-xscrollcommand {.fsensor.sx2 set} -yscrollcommand {.fsensor.sy2 set}" +
		" -selectforeground white -takefocus 0 -height 30 -width 30" + font,
	"scrollbar .fsensor.sx2 -orient horizontal -command {.fsensor.l2 xview}",
	"scrollbar .fsensor.sy2 -command {.fsensor.l2 yview}",
	"label .fsensor.l -text {Step 1: Select Sensors} -height 30"+fontb,
	"frame .fsensor.fbut",
	"grid .fsensor.bin -in .fsensor.fbut -column 0 -row 0 -padx 10 -pady 5",
	"grid .fsensor.bout -in .fsensor.fbut -column 0 -row 1 -padx 10 -pady 5",

	"grid .fsensor.l -row 0 -column 0 -sticky w -columnspan 5",
	"grid .fsensor.sy1 -column 0 -row 2 -sticky ns -rowspan 2",
	"grid .fsensor.sx1 -column 1 -row 3 -sticky ew",
	"grid .fsensor.l1 -column 1 -row 2 -sticky nsew",
	"grid .fsensor.fbut -column 2 -row 2 -rowspan 2",
	"grid .fsensor.sy2 -column 3 -row 2 -sticky ns -rowspan 2",
	"grid .fsensor.sx2 -column 4 -row 3 -sticky ew",
	"grid .fsensor.l2 -column 4 -row 2 -sticky nsew",
	"grid rowconfigure .fsensor 2 -weight 1",
	"grid columnconfigure .fsensor 1 -weight 1",
	"grid columnconfigure .fsensor 4 -weight 1",

	"grid .fsensor -in .f -row 0 -column 0 -sticky nsew",
};

makegraphwin(ctxt: ref Draw->Context, C: ref Context, data: array of array of int)
{
	(top, titlectl) := tkclient->toplevel(ctxt, "", "Create Graph", tkclient->Appl);
	butchan := chan of string;
	tk->namechan(top, butchan, "butchan");
	tkcmds(top, makegraphscr);
	selection := array[len C.sensor] of (int, string);
	for (i := 0; i < len selection; i++) {
		selection[i] = (1, string C.sensor[i].id+": "+C.sensor[i].loc);
		tkcmd(top, ".fsensor.l1 insert end {"+selection[i].t1+"}");
	}
	tkcmd(top, "pack propagate . 0; update");
	tkcmd(top, ". configure -width 350 -height 350");
	tkclient->onscreen(top, nil);
	tkclient->startinput(top, "kbd"::"ptr"::nil);
	sensors: array of int;
	starttime := C.starttime;
	endtime := C.endtime;
	mintime := C.starttime;
	maxtime := C.endtime;
	
	ltime := maxtime - mintime ;
	tkcmd(top, ".ftime.lstart configure -text {" + formattime(starttime, TM1LINE) + "}");
	tkcmd(top, ".ftime.lend configure -text {" + formattime(endtime, TM1LINE) + "}");
	tkcmd(top, ".ftime.llen configure -text {" +
		formattime(endtime - starttime, LENTIME) + "}");

	for (;;) alt {
		s := <-top.ctxt.kbd =>
			tk->keyboard(top, s);
		s := <-top.ctxt.ptr =>
			tk->pointer(top, *s);
		inp := <-butchan =>
			lst := str->unquoted(inp);
			case hd lst {
			"cancel" =>
				return;
			"draw" =>
				sensors = array[int tkcmd(top, ".fsensor.l2 size")] of int;
				j := 0;
				for (i = 0; i < len selection; i++)
					if (selection[i].t0 == 2)
						sensors[j++] = i;
				hrstart := (starttime - mintime) / 3600;
				hrend := 1 + (endtime - mintime) / 3600;
				spawn graphwin(ctxt, C, DB, starttime, data[hrstart: hrend],
					sensors, "Sensor Graph", graphscr);
				return;
			"back" =>
				tkcmd(top, "grid forget .ftime");
				tkcmd(top, "grid .fsensor -in .f -row 0 -column 0 -sticky snew");
				tkcmd(top, "grid forget .f.fbut.bb .f.fbut.bd");
				tkcmd(top, "grid .f.fbut.bn -row 0 -column 1 -pady 10");
			"next" =>
				tkcmd(top, "grid forget .fsensor");
				tkcmd(top, "grid .ftime -in .f -row 0 -column 0 -sticky new");

				tkcmd(top, "grid forget .f.fbut.bn");
				tkcmd(top, "grid .f.fbut.bd -row 0 -column 1 -pady 10");
				tkcmd(top, "grid .f.fbut.bb -row 0 -column 0 -padx 10 -pady 10");
			"set" =>
				w := real tkcmd(top, ".ftime.fdisp cget -actwidth");
				ox := int tkcmd(top, ".ftime.fdisp cget -actx");
				x := int hd tl tl lst - ox;
				f := real x / w;
				if (hd tl lst == "start") {
					starttime = roundto(mintime + int (real ltime * f), HOUR);
					if (starttime < mintime)
						starttime = mintime;
					if (starttime > endtime)
						starttime = endtime;
					tkcmd(top, ".ftime.lstart configure -text {" +
						formattime(starttime, TM1LINE) + "}");
				}
				else if (hd tl lst == "end") {
					endtime = roundto(mintime + int (real ltime * f), HOUR);
					if (endtime > maxtime)
						endtime = maxtime;
					if (endtime < starttime)
						endtime = starttime;
					tkcmd(top, ".ftime.lend configure -text {" +
						formattime(endtime, TM1LINE) + "}");
				}
				tkcmd(top, ".ftime.llen configure -text {" +
					formattime(endtime - starttime, LENTIME) + "}");

				tkcmd(top, "grid columnconfigure .ftime.fdisp 0 -weight "+
					string ((starttime - mintime) / 3600));
				tkcmd(top, "grid columnconfigure .ftime.fdisp 2 -weight "+
					string ((endtime - starttime) / 3600));
				tkcmd(top, "grid columnconfigure .ftime.fdisp 4 -weight "+
					string ((maxtime - endtime) / 3600));
				if (endtime - starttime == 0)
					tkcmd(top, ".f.fbut.bd configure -state disabled");
				else
					tkcmd(top, ".f.fbut.bd configure -state normal");
			"move" =>
				nto := int hd tl tl lst;
				wfrom := ".fsensor.l" + string hd tl lst;
				lsel := str->unquoted(tkcmd(top, wfrom+" curselection"));
				i = 0;
				for (; lsel != nil; lsel = tl lsel) {
					s := tkcmd(top, wfrom + " get "+hd lsel);
					for (;;) {
						if (s == selection[i].t1)
							break;
						i++;
					}
					selection[i].t0 = nto;
				}
				tkcmd(top, ".fsensor.l1 selection clear 0 end");
				tkcmd(top, ".fsensor.l2 selection clear  0 end");
				tkcmd(top, ".fsensor.l1 delete 0 end");
				tkcmd(top, ".fsensor.l2 delete 0 end");
				for (i = 0; i < len selection; i++)
					tkcmd(top, ".fsensor.l"+string selection[i].t0+
						" insert end {"+selection[i].t1+"}");
				if (int tkcmd(top, ".fsensor.l2 size") > 0)
					tkcmd(top, ".f.fbut.bn configure -state normal");
				else
					tkcmd(top, ".f.fbut.bn configure -state disabled");

			}
			tkcmd(top, "update");
		s := <-top.ctxt.ctl or
		s = <-top.wreq or
		s = <- titlectl =>
			if (s == "exit")
				return;
			tkclient->wmctl(top, s);
	}
}

graphscr := array[] of {
	"frame .f -bg white",
	"canvas .f.c -bg white -height 50 -width 10 -borderwidth 0",
	"panel .f.p",
	".f.c create window 0 0 -window .f.p -anchor nw",
	"frame .f.fkey -bg white",
	"scrollbar .f.fkey.sy -command {.f.fkey.c yview}",
	"scrollbar .f.fkey.sx -command {.f.fkey.c xview} -orient horizontal",
	"canvas .f.fkey.c -height 50 -width 20 -xscrollcommand {.f.fkey.sx set} "+
		"-yscrollcommand {.f.fkey.sy set} -bg white -borderwidth 0",
	"frame .fkey -bg white",
	".f.fkey.c create window 0 0 -window .fkey -anchor nw",

	"pack .f -fill both -expand 1",
	"grid .f.c .f.fkey -row 0 -sticky nsew",
	"grid rowconfigure .f 0 -weight 1",
	"grid columnconfigure .f 0 -weight 5",
	"grid columnconfigure .f 1 -weight 1",

	"pack .f.fkey.sy -side left -fill y",
	"pack .f.fkey.sx -side bottom -fill x",
	"pack .f.fkey.c -side right -fill both -expand 1",

};

graphwin(ctxt: ref Draw->Context, C: ref Context, graph, starttime: int, data: array of array of int, sensors: array of int, title: string, tkc: array of string)
{
	(top, titlectl) := tkclient->toplevel(ctxt, "", title, tkclient->Appl);
	butchan := chan of string;
	tk->namechan(top, butchan, "butchan");
	tkcmds(top, tkc);

	labels := array[len sensors] of string;
	for (i := 0; i < len labels; i++)
		labels[i] = string sensors[i] + ": " + C.sensor[sensors[i]].loc;
	tkcmd(top, "pack propagate . 0; update");
	tkcmd(top, ". configure -width 500 -height 300");
	if (graph == DB)
		drawgraph(top, C, starttime, data, sensors, labels);
	else
		drawbatlife(top, data[0], sensors, labels);
	tkclient->onscreen(top, nil);
	tkclient->startinput(top, "kbd"::"ptr"::nil);
	for (;;) alt {
		s := <-top.ctxt.kbd =>
			tk->keyboard(top, s);
		s := <-top.ctxt.ptr =>
			tk->pointer(top, *s);
		inp := <-butchan =>
			if (inp == "excel")
				spawn export2excel(ctxt, starttime, data, sensors, labels);
		s := <-top.ctxt.ctl or
		s = <-top.wreq or
		s = <- titlectl =>
			if (s == "exit")
				return;
			tkclient->wmctl(top, s);
			(nil, lst) := sys->tokenize(s, " \t\n");
			if (len lst > 1 && hd lst == "!size" && hd tl lst == ".") {
				if (graph == DB)
					drawgraph(top, C, starttime, data, sensors, labels);
				else
					drawbatlife(top, data[0], sensors, labels);
			}
	}
}

delnewline(s: string): string
{
	s2 := "";
	for (i := 0; i < len s; i++) {
		if (s[i] == '\n')
			s2 += "\\n";
		else
			s2[len s2] = s[i];
	}
	return s2;
}

addnewline(s: string): string
{
	s2 := "";
	for (i := 0; i < len s; i++) {
		if (s[i] == '\\' && i + 1 < len s && s[i+1] == 'n') {
			s2[len s2] = '\n';
			i++;
		}
		else
			s2[len s2] = s[i];
	}
	return s2;
}

makeupdata(nsensors, nhours: int): (int, array of array of int)
{
	# nhours++;
	starttm := daytime->local(daytime->now());
	starttm.min = 0;
	starttm.sec = 0;
	starttm.hour = 0;
	starttime := daytime->tm2epoch(starttm);
	data := array[nhours] of { * => array[nsensors] of int };
	for (i := 0; i < nsensors; i++)
		data[0][i] = rand->rand(maxdb);
	for (hour := 1; hour < nhours; hour++) {
		for (sensor := 0; sensor < nsensors; sensor++) {
			mv := rand->rand(3);
			if (rand->rand(6) == 0)
				mv += rand->rand(4);
			mv *= (2*rand->rand(2)) - 1;
			if (rand->rand(40) == 0)
				data[hour][sensor] = -1;
			else {
				if (data[hour-1][sensor] == -1)
					data[hour][sensor] = data[hour-2][sensor] + mv;
				else
					data[hour][sensor] = data[hour-1][sensor] + mv;
				if (data[hour][sensor] < 0)
					data[hour][sensor] = 0;
				if (data[hour][sensor] > maxdb)
					data[hour][sensor] = maxdb;
			}
		}
	}
	return (starttime, data);
}

getlimit(C: ref Context, tm: ref Daytime->Tm): int
{
	limitid := 0;
	for (i := 1; i < len C.limits; i++) {
		if (daymatch(C.limits[i].day, tm.wday) &&
			hourmatch(C.limits[i].start, C.limits[i].end, tm.hour)) {
			limitid = i;
			break;
		}
	}
	return limitid;
}

graphcol := array[] of {
	int 16rff0000ff,
	int 16rff8000ff,
	int 16rffff00ff,
	int 16raaff00ff,
	int 16r00ff80ff,
	int 16r00ffffff,
	int 16r0000ffff,
	int 16r8000ffff,
	int 16rff66ffff,
};

pix2sensor(C: ref Context, p: Point): int
{
	pc := pix2coord(p, C);
	r := Rect (pix2coord(p.sub((10,-10)), C), pix2coord(p.add((10,-10)), C));
	selected := -1;
	dist := -1;
	for (tmp := C.dispsensorl; tmp != nil; tmp = tl tmp) {
		if (C.sensor[hd tmp].coord.in(r)) {
			dp := C.sensor[hd tmp].coord.sub(pc);
			d := abs(dp.x) + abs(dp.y);
			if (dist == -1 || d < dist) {
				dist = d;
				selected = hd tmp;
			}
		}
	}
	return selected;
}

selectsensor(top: ref Tk->Toplevel, C: ref Context, sensor, mode: int)
{
	if (sensor == -1)
		return;
	selected := C.sensor[sensor].selected;
	if (mode == TOGGLE)
		C.sensor[sensor].selected = ++C.sensor[sensor].selected % 2;
	else if (mode == SELECT)
		C.sensor[sensor].selected = 1;
	else
		C.sensor[sensor].selected = 0;
	C.nselected += C.sensor[sensor].selected - selected;
	psensor := coord2pix(C.sensor[sensor].coord, C);
	if (C.sensor[sensor].selected) {
	#	tkcmd(top, ".fsensor.l configure -text {Sensor "+
	#		string C.sensor[sensor].id+": "+
	#		C.sensor[sensor].loc+"}");
		C.mapimg.fillellipse(psensor, 2,2,display.rgb(0,255,0), (0,0));
	}
	else
		C.mapimg.fillellipse(psensor, 2,2,display.rgb(255,0,0), (0,0));
	#if (C.nselected != 1)
	#	tkcmd(top, ".fsensor.l configure -text {Sensor}");

	C.mapimg.ellipse(psensor, 2,2,0,display.black, (0,0));
	tkcmd(top, sys->sprint(".pmap dirty %d %d %d %d",
		psensor.x - 3, psensor.y - 3, psensor.x + 3, psensor.y + 3));
}

inlist(l: list of int, i: int): int
{
	for (tmp := l; tmp != nil; tmp = tl tmp)
		if (hd tmp == i)
			return 1;
	return 0;
}

rad2deg(r: real): int
{
	return int (r * 180.0 / 3.1415927);
}

deg2rad(d: int): real
{
	return (real d  * 3.1415927 / 180.0);
}

MIN: con 60;
HOUR: con MIN * 60;
DAY: con HOUR * 24;
WEEK: con DAY * 7;

roundto(t, round: int): int
{
	tm := daytime->local(t);
	if (round >= MIN)
		tm.sec = 0;
	if (round >= HOUR)
		tm.min = 0;
	if (round >= DAY)
		tm.hour = 0;
	if (round >= WEEK)
		tm.wday = 0;
	return daytime->tm2epoch(tm);
}

months := array[] of {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec",
};

monthdays := array[] of {
	31,
	29,
	31,
	30,
	31,
	30,
	31,
	31,
	30,
	31,
	30,
	31,
};

projectscr := array[] of {
	"frame .f",
	"label .f.l -text {Project Details}"+fontb,
	"label .f.lname -text {Name:}"+font,
	"entry .f.ename -bg white -width 50"+font,
	"label .f.lstart -text {Start date:}"+font,
	"entry .f.estartday -width 30 -bg white"+font,
	"choicebutton .f.cbstartmon"+font,
	"choicebutton .f.cbstartyr"+font,
	"label .f.lend -text {End date:}"+font,
	"entry .f.eendday -width 30 -bg white"+font,
	"choicebutton .f.cbendmon"+font,
	"choicebutton .f.cbendyr"+font,
	"label .f.lnotes -text {Notes:}"+font,
	"label .f.lfrom -text {From:}"+font,
	"scrollbar .f.tsy -command {.f.tnotes yview}",
	"text .f.tnotes -bg white -yscrollcommand {.f.tsy set} -height 20 -width 50"+font,
	"frame .f.fnotes",
	"pack .f.tsy -in .f.fnotes -side left -fill y",
	"pack .f.tnotes -in .f.fnotes -side left -fill both -expand 1",
	"bind .f.ename {<Key-	>} {focus .f.estartday}",
	"bind .f.estartday {<Key-	>} {focus .f.eendday}",
	"bind .f.eendday {<Key-	>} {focus .f.tnotes}",
	"bind .f.tnotes {<Key-	>} {send butchan notestab}",

	"button .f.bok -text { Ok } -takefocus 0 -command {send butchan ok}"+font,
	"button .f.bcancel -text {Cancel} -takefocus 0 -command {send butchan cancel}"+font,
	"frame .f.fbut",
	"grid .f.bok .f.bcancel -row 0 -in .f.fbut -padx 10 -pady 10",

	"label .f.lset -text {Sensor Settings}"+fontb,
	"canvas .f.c -yscrollcommand {.f.sy set} -height 50 -width 50",
	"scrollbar .f.sy -command {.f.c yview}",

	"label .f.lsample -text {Sample:}"+font,
	"choicebutton .f.cbsample -values {{By time} {By interval}} "+
		"-command {send butchan sampletype}"+font,

	"frame .f.fsensor",
	"grid .f.sy -in .f.fsensor -row 0 -column 0 -sticky ns",
	"grid .f.c -in .f.fsensor -row 0 -column 1 -sticky nsew",
	"grid rowconfigure .f.fsensor 0 -weight 1",
	"grid columnconfigure .f.fsensor 1 -weight 1",
	"frame .f.fperiod",
	"radiobutton .f.fperiod.rb1 -text {Continuously} -variable sample -value 0 -takefocus 0 "+
		"-command {.f.fperiod.ep configure -state disabled -bg #dddddd}"+font,
	"radiobutton .f.fperiod.rb2 -text {Every} -variable sample -value 1 -takefocus 0 "+
		"-command {.f.fperiod.ep configure -state normal -bg white}"+font,
	"variable sample 0",
	"entry .f.fperiod.ep -width 30 -bg #dddddd -state disabled"+font,
	"bind .f.fperiod.ep {<Key-	} {focus .fsenset.efrom0}",
	"choicebutton .f.fperiod.cbp -values {{hours} {minutes} {seconds}}"+font,
	"grid .f.fperiod.rb1 -row 0 -column 0 -columnspan 3 -sticky w",
	"grid .f.fperiod.rb2 .f.fperiod.ep .f.fperiod.cbp -row 1 -sticky w",
	"frame .fsenset",
	"button .fsenset.bnew -text {more} -command {send butchan more} -takefocus 0"+font,
	".f.c create window 0 0 -window .fsenset -anchor nw",
	"checkbutton .f.cblocal -text {Standalone (log data to flash memory)} "+
		"-takefocus 0 -variable standalone -anchor w -width 0"+font,

	"frame .f.fstart",
	"frame .f.fend",
	"grid .f.estartday .f.cbstartmon .f.cbstartyr -row 0 -in .f.fstart",
	"grid .f.eendday .f.cbendmon .f.cbendyr -row 0 -in .f.fend",

	"grid .f.l -row 0 -column 0 -sticky w -columnspan 2 -pady 5",
	"grid .f.lname -row 2 -column 0 -sticky w",
	"grid .f.ename -row 2 -column 1 -sticky ew",
	"grid .f.lstart .f.fstart -row 3 -sticky w",
	"grid .f.lend .f.fend -row 4 -sticky w",
	"grid .f.lnotes -row 6 -column 0 -sticky nw",
	"grid .f.fnotes -row 6 -column 1 -sticky nsew",
	"grid .f.lset -row 7 -column 0 -sticky w -columnspan 2 -pady 5",
	"grid .f.lsample .f.fperiod -row 8 -sticky nw",
	"grid .f.lfrom -row 9 -column 0 -sticky nw",
	"grid .f.fsensor -row 9 -column 1 -sticky nsew",
	"grid .f.cblocal -row 10 -column 1 -sticky ew",
	"grid .f.fbut -row 11 -column 0 -columnspan 2",

	"grid rowconfigure .f 6 -weight 1",
	"grid rowconfigure .f 9 -weight 1",
	"grid columnconfigure .f 1 -weight 1",
	"pack .f -fill both -expand 1",
};

newtimep(top: ref Tk->Toplevel, i: int)
{
	tk->cmd(top, "grid forget .fsenset.bnew");
	si := string i;
	tkcmd(top, "entry .fsenset.efrom"+si+" -bg white -width 60"+font);
	tkcmd(top, "entry .fsenset.eto"+si+" -bg white -width 60"+font);
	tkcmd(top, "label .fsenset.lto"+si+" -text { to }"+font);
	tkcmd(top, "grid .fsenset.efrom"+si+" .fsenset.lto"+si+
		" .fsenset.eto"+si+" -sticky w -row "+si);
	tkcmd(top, ".fsenset.efrom"+si+" insert insert {hh:mm}");
	tkcmd(top, ".fsenset.eto"+si+" insert insert {hh:mm}");
	tkcmd(top, "bind .fsenset.efrom"+si+" {<Key-	} {focus .fsenset.eto"+si+"}");
	tkcmd(top, "bind .fsenset.eto"+si+" {<Key-	} {send butchan mvfocus "+si+"}");
	tkcmd(top, "grid .fsenset.bnew -row "+si+" -column 3");
	h := tkcmd(top, ".fsenset cget -height");
	tkcmd(top, ".f.c configure -scrollregion {0 0 " +
		tkcmd(top, ".fsenset cget -width") + " " + h + "}");
	tkcmd(top, ".f.c see 0 "+h);
}

Project: adt {
	name, notes: string;
	id, startdate, enddate, sampletype, period, periodtype, standalone: int;
	lonoff: list of (int, int);
};

loadproject(id: int, C: ref Context): string
{
	pNAME, pNOTES, pSTART, pEND, pTSAMPLE, pPERIOD, pTPERIOD, pSTANDALONE: con iota;
	sID, sLOC, sNOTES, sCX, sCY, sDX, sDY, sHEIGHT, sWALL: con iota;
	iobuf := bufio->open(datapath+"/" + string id + ".proj", bufio->OREAD);
	if (iobuf == nil)
		return sys->sprint("Error: could not open %s/%d.proj: %r",datapath, id);

	s := iobuf.gets('\n');
	if (s == nil)
		return "Empty project file";
	a := list2array(str->unquoted(s));
	if (len a > pSTANDALONE)
		C.project = ref Project(addnewline(a[pNAME]), addnewline(a[pNOTES]), id,
				int a[pSTART], int a[pEND], int a[pTSAMPLE],
				int a[pPERIOD], int a[pTPERIOD], int a[pSTANDALONE], nil);
	else
		return "Invalid project file";
	s = iobuf.gets('\n');
	if (s == nil)
		return "Invalid project file";
	lst := str->unquoted(s);
	for (; lst != nil && tl lst != nil; lst = tl tl lst)
		C.project.lonoff = (int hd lst, int hd tl lst) :: C.project.lonoff;

	lsensor: list of Sensor = nil;
	for (;;) {
		s = iobuf.gets('\n');
		if (s == nil)
			break;
		lst = str->unquoted(s);
		if (len lst >= 5) {
			a = list2array(lst);
			lsensor = Sensor (int a[sID], addnewline(a[sLOC]), addnewline(a[sNOTES]),
						(int a[sCX], int a[sCY]),
						(int a[sDX], int a[sDY]), 0, 0, int a[sHEIGHT],
						int a[sWALL], 0, 0) :: lsensor;
		}
	}
	C.sensor = array[len lsensor] of Sensor;
	i := len C.sensor;
	for (; lsensor != nil; lsensor = tl lsensor)
		C.sensor[--i] = hd lsensor;
	return nil;
}

saveproject(C: ref Context): int
{
	if (C.project == nil)
		return 0;
	sys->print("Saving Project %d\n", C.project.id);
	fd := sys->create(datapath + "/" + string C.project.id + ".proj", sys->OWRITE, 8r666);
	if (fd == nil) {
		sys->print("Error writing project file: %s/%d.proj: %r\n", datapath, C.project.id);
		return -1;
	}
	
	sp := str->quoted(delnewline(C.project.name) :: delnewline(C.project.notes) :: nil) +
		sys->sprint(" %d %d %d %d %d %d\n", C.project.startdate, C.project.enddate,
		C.project.sampletype, C.project.period, C.project.periodtype, C.project.standalone);
	for (tmp := C.project.lonoff; tmp != nil; tmp = tl tmp) {
		(on, off) := hd tmp;
		sp += string on + " " + string off +" ";
	}
	sys->fprint(fd, "%s\n", sp);

	for (i := 0; i < len C.sensor; i++) {
		s := C.sensor[i];
		ls := string s.id :: delnewline(s.loc) :: delnewline(s.notes) ::
			string s.coord.x :: string s.coord.y :: string s.dispc.x :: 
			string s.dispc.y :: string s.height :: string s.wall :: nil;
		sys->fprint(fd, "%s\n", str->quoted(ls));
	}
	return 0;
}

projectwin(ctxt: ref Draw->Context, proj: ref Project, projchan: chan of ref Project)
{
	(top, titlectl) := tkclient->toplevel(ctxt, "", "New Project", tkclient->Appl);
	butchan := chan of string;
	tk->namechan(top, butchan, "butchan");
	tkcmds(top, projectscr);
	tm := daytime->local(daytime->now());
	mon := "{";
	for (j := 0; j < len months; j++)
		mon += "{" + months[j] + "}";
	mon += "}";
	tkcmd(top, ".f.cbstartmon configure -values "+mon);
	tkcmd(top, ".f.cbendmon configure -values "+mon);
	tkcmd(top, ".f.cbstartmon set "+string tm.mon);
	tkcmd(top, ".f.cbendmon set "+string tm.mon);
	year := "{";
	for (j = 0; j < 5; j++)
		year += "{" + string (1900 + tm.year + j) + "}";
	year += "}";
	tkcmd(top, ".f.cbstartyr configure -values "+year);
	tkcmd(top, ".f.cbendyr configure -values "+year);

	nrows := 0;
	newtimep(top, nrows++);

	if (proj != nil) {
		tkcmd(top, ".f.ename insert 0 {"+proj.name+"}");
		tkcmd(top, ".f.tnotes insert 1.0 '"+proj.notes);
		(d, m, y) := int2date(proj.startdate);
		tkcmd(top, ".f.estartday insert 0 {"+string d + "}");
		tkcmd(top, ".f.cbstartmon set "+string m);
		tkcmd(top, ".f.cbstartyr set "+string (y - tm.year - 1900));

		if (proj.enddate != -1) {
			(d, m, y) = int2date(proj.enddate);
			tkcmd(top, ".f.eendday insert 0 {"+string d + "}");
			tkcmd(top, ".f.cbendmon set "+string m);
			tkcmd(top, ".f.cbendyr set "+string (y - tm.year - 1900));
		}
		tkcmd(top, "variable sample "+string proj.sampletype);
		if (proj.sampletype == 1) {
			tkcmd(top, ".f.fperiod.ep insert 0 {"+string proj.period+"}");
			tkcmd(top, ".f.fperiod.ep configure -bg white");			
		}
		tkcmd(top, ".f.fperiod.cbp set "+string proj.periodtype);
		for (tmp := proj.lonoff; tmp != nil; tmp = tl tmp) {
			(on, off) := hd tmp;
			sid := string (nrows - 1);
			tkcmd(top, ".fsenset.efrom"+ sid + " delete 0 end");
			tkcmd(top, ".fsenset.efrom"+ sid + " insert 0 {" + inttime2strtime(on) + "}");
			tkcmd(top, ".fsenset.eto"+ sid + " delete 0 end");
			tkcmd(top, ".fsenset.eto"+ sid + " insert 0 {" + inttime2strtime(off) + "}");
			newtimep(top, nrows++);
		}
		tkcmd(top, "variable standalone "+string proj.standalone);
	}

	tkcmd(top, "pack propagate . 0; update");
	tkcmd(top, ". configure -width 350 -height 350");
	tkclient->onscreen(top, nil);
	tkclient->startinput(top, "kbd"::"ptr"::nil);

	for (;;) alt {
		s := <-top.ctxt.kbd =>
			tk->keyboard(top, s);
		s := <-top.ctxt.ptr =>
			tk->pointer(top, *s);
		inp := <-butchan =>
			lst := str->unquoted(inp);
			case hd lst {
				"cancel" =>
					return;
				"ok" =>
					(p, failed) := getprojectdata(top);
					if (failed == nil) {
						if (proj == nil)
							p.id = getnewprojectid();
						else
							p.id = proj.id;
						projchan <-= p;
						return;
					}
					tkcmd(top, "focus "+hd failed);
					for (; failed != nil; failed = tl failed)
						tkcmd(top, hd failed+" configure -bg yellow");
				"more" =>
					newtimep(top, nrows++);

				"notestab" =>
					if (tkcmd(top, "variable period") == "1")
						tkcmd(top, "focus .f.fperiod.ep");
					else
						tkcmd(top, "focus .fsenset.efrom0; .f.c see 0 0");

				"mvfocus" =>
					row := int hd tl lst;
					if (row == nrows - 1) {
						sfrom := tkcmd(top, ".fsenset.efrom"+string row+" get");
						sto := tkcmd(top, ".fsenset.eto"+string row+" get");
						if ((sfrom == "hh:mm" || sfrom == nil) &&
							(sto == "hh:mm" || sto == nil))
							tkcmd(top, "focus .f.ename");
						else {
							newtimep(top, nrows++);
							tkcmd(top, "focus .fsenset.efrom" + string (row + 1));
						}
					}
					else {
						tkcmd(top, ".f.c see 0 "+string ((row + 2) *
							(4 + int tkcmd(top, ".f.fperiod.ep cget -height"))));
						tkcmd(top, "focus .fsenset.efrom" + string (row + 1));
					}
			}
			tkcmd(top, "update");
		s := <-top.ctxt.ctl or
		s = <-top.wreq or
		s = <- titlectl =>
			if (s == "exit")
				return;
			tkclient->wmctl(top, s);
	}
}

date2int(day, month, year: int): int
{
	return day | month << 6 | year << 10;
}

int2date(i: int): (int, int, int)
{
	return (i & 31, (i >> 6) & 15, i >> 10);
}

strtime2inttime(s: string): int
{
	hrmin := array[2] of { * => "" };
	n := 0;
	for (i := 0; i < len s; i++) {
		case s[i] {
		'0' to '9' =>
			hrmin[n][len hrmin[n]] = s[i];
		':' or '.' =>
			n++;
			if (n > 1)
				return -1;
		'\n' =>;
		* =>
			return -1;
		}
	}
	hr := int hrmin[0];
	min := int hrmin[1];
	if (hr > 23)
		return -1;
	if (min > 59)
		return -1;
	return time2int(hr, min);	
}

time2int(hrs, mins: int): int
{
	return hrs << 7 | mins;
}

int2time(i: int): (int, int)
{
	return (i >> 7, i & 63);
}

inttime2strtime(i: int): string
{
	(hrs, mins) := int2time(i);
	return sys->sprint("%.2d:%.2d", hrs, mins);
}

getprojectdata(top: ref Tk->Toplevel): (ref Project, list of string)
{
	failed: list of string = nil;
	p: Project;
	p.name = getentry(top, ".f.ename");
	if (p.name == nil)
		failed = ".f.ename" :: failed;
	p.notes = tkcmd(top, ".f.tnotes get 1.0 end");
	startday := int getentry(top, ".f.estartday");
	startmon := int tkcmd(top, ".f.cbstartmon get");
	endday := int getentry(top, ".f.eendday");
	endmon := int tkcmd(top, ".f.cbendmon get");
	if (startday < 1 || startday > monthdays[startmon])
		failed = ".f.estartday" :: failed;
	if (endday < 1 || endday > monthdays[endmon])
		failed = ".f.eendday" :: failed;
	p.startdate = date2int(startday, startmon,
		int tkcmd(top, ".f.cbstartyr getvalue"));
	p.enddate = date2int(endday, endmon,
		int tkcmd(top, ".f.cbendyr getvalue"));
	p.sampletype = int tkcmd(top, "variable sample");
	period := getentry(top, ".f.fperiod.ep");
	if (p.sampletype == 1 && (period == nil || int period < 1))
		failed = ".f.fperiod.ep" :: failed;
	else
		tkcmd(top, ".f.fperiod.ep configure -bg #dddddd");
	p.period = int period;
	p.periodtype = int tkcmd(top, ".f.fperiod.cbp get");
	p.standalone = int tkcmd(top, "variable standalone");
	lslaves := str->unquoted(tkcmd(top, "grid slaves .fsenset -column 0"));

	for (; lslaves != nil; lslaves = tl lslaves) {
		id := (hd lslaves)[len ".fsenset.efrom":];
		tfrom := getentry(top, ".fsenset.efrom"+id);
		tto := getentry(top, ".fsenset.eto"+id);
		if (tfrom == "hh:mm")
			tfrom = nil;
		if (tto == "hh:mm")
			tto = nil;
		if (tfrom == nil && tto == nil)
			continue;
		if (tfrom == nil)
			failed = ".fsenset.efrom"+id :: failed;
		else if (tto == nil)
			failed = ".fsenset.eto"+id :: failed;
		else {
			on := strtime2inttime(tfrom);
			off := strtime2inttime(tto);
			if (on == -1)
				failed = ".fsenset.efrom"+id :: failed;
			if (off == -1)
				failed = ".fsenset.eto"+id :: failed;
			else if (on != -1)
				p.lonoff = (on, off) :: p.lonoff;
		}
	}
	return (ref p, failed);
}

getentry(top: ref Tk->Toplevel, widget: string): string
{
	tkcmd(top, widget + " configure -bg white");
	return tkcmd(top, widget + " get");
}

getnewprojectid(): int
{
	(dirs, nil) := readdir->init(datapath, readdir->NAME);
	lid: list of int = nil;
	for (i := 0; i < len dirs; i++) {
		l := len dirs[i].name - 5;
		if (l > 0 && dirs[i].name[l:] == ".proj")
			lid = int dirs[i].name[:l] :: lid;
	}
	pid := -1;
	for (; lid != nil; lid = tl lid)
		if (hd lid > pid)
			pid = hd lid;
	return pid + 1;
}

loadprojectscr := array[] of {
	"frame .f",
	"scrollbar .f.sy -command {.f.t yview}",
	"scrollbar .f.sx -orient horizontal -command {.f.t xview}",

	"frame .f.fbut",
	"button .f.fbut.bok -text { Load } -takefocus 0 -command {send butchan ok}"+font,
	"button .f.fbut.bcancel -text {Cancel} -takefocus 0 -command {send butchan cancel}"+font,
	"listbox .f.lb -height 50 -width 50 -xscrollcommand {.f.sx set} "+
		"-yscrollcommand {.f.sy set} -selectmode single -selectforeground white"+
		" -selectbackground blue -takefocus 0 -bg white"+font,
	"bind .f.lb <Double-Button-1> {send butchan double %y}",

	"grid .f.fbut.bok .f.fbut.bcancel -row 0 -padx 10 -pady 10",
	"grid .f.sy -row 0 -column 0 -rowspan 2 -sticky ns",
	"grid .f.sx -row 1 -column 1 -sticky ew",
	"grid .f.lb -row 0 -column 1 -sticky nsew",
	"grid .f.fbut -row 2 -column 0 -columnspan 2",
	"grid rowconfigure .f 0 -weight 1",
	"grid columnconfigure .f 1 -weight 1",
	"pack .f -fill both -expand 1",
	"pack propagate . 0",
};

loadprojectwin(ctxt: ref Draw->Context, chanout: chan of string)
{
	(top, titlectl) := tkclient->toplevel(ctxt, "", "Load Project", tkclient->Appl);
	butchan := chan of string;
	tk->namechan(top, butchan, "butchan");
	tkcmds(top, loadprojectscr);
	tkcmd(top, "pack propagate . 0; update");
	tkcmd(top, ". configure -height 300 -width 200");

	projects := getprojectnames();
	for (i := 0; i < len projects; i++)
		tkcmd(top, ".f.lb insert end {"+projects[i].name+"}");
	selected := -1;
	tkclient->onscreen(top, nil);
	tkclient->startinput(top, "kbd"::"ptr"::nil);
	for (;;) alt {
		s := <-top.ctxt.kbd =>
			tk->keyboard(top, s);
		s := <-top.ctxt.ptr =>
			tk->pointer(top, *s);
		inp := <-butchan =>
			lst := str->unquoted(inp);
			case hd lst {
			"ok" =>
				select := tkcmd(top, ".f.lb curselection");
				if (select != nil) {
					chanout <-= "project load "+string projects[int select].id;
					return;
				}
			"cancel" =>
				return;
			"double" =>
				select := int tkcmd(top, ".f.lb nearest " + hd tl lst);
				if (select >= 0 && select < len projects) {
					chanout <-= "project load "+string projects[int select].id;
					return;
				}
			}
			tkcmd(top, "update");
		s := <-top.ctxt.ctl or
		s = <-top.wreq or
		s = <- titlectl =>
			if (s == "exit")
				return;
			tkclient->wmctl(top, s);
	}
}

ProjectInfo: adt {
	name: string;
	id: int;
};

getprojectnames(): array of ref ProjectInfo
{
	lnames: list of ref ProjectInfo= nil;
	(dirs, nil) := readdir->init(datapath, readdir->NAME | readdir->DESCENDING);
	for (i := 0; i < len dirs; i++) {
		l := len dirs[i].name - 5;
		if (l > 0 && dirs[i].name[l:] == ".proj") {
			iobuf := bufio->open(datapath+"/" + dirs[i].name, bufio->OREAD);
			if (iobuf == nil)
				sys->print("Error: could not open %s/%s: %r",datapath, dirs[i].name);
			else {
				lst := str->unquoted(iobuf.gets('\n'));
				if (lst == nil)
					continue;
				lnames = ref ProjectInfo (addnewline(hd lst), int dirs[i].name[:l+1]) :: lnames;
			}
		}
	}
	return list2array(lnames);
}

excelpath := "c:\\program files\\Microsoft Office\\Office10\\excel.exe";
export2excel(ctxt: ref Draw->Context, starttime: int, data: array of array of int, sensors: array of int, labels: array of string)
{
	emuroot := env->getenv("emuroot");
	filename: string;
	fd: ref sys->FD;

	for (i := 0;;) {
		filename = sys->sprint("/tmp/~tmp%.4d.csv", i++);
		(n, nil) := sys->stat(filename);
		fd = sys->create(filename, sys->OWRITE | sys->OTRUNC, 8r666);
		if (fd != nil)
			break;
	}
	endtime := starttime + (HOUR * len data);
	sys->fprint(fd, "%s - %s\n\n", formattime(starttime, TMSHORT),
		formattime(endtime, TMSHORT));
	sys->fprint(fd, ", ");
	for (i = 0; i < len labels; i++)
		sys->fprint(fd, "%s,", labels[i]);
	sys->fprint(fd, "\n");

	time := starttime;
	for (i = 0; i < len data; i++) {
		tm := daytime->local(time);
		if (tm.hour == 0 || i == 0)
			sys->fprint(fd, "%s,", formattime(time, TMSHORT));
		else
			sys->fprint(fd, "%.2d:%.2d,", tm.hour, tm.min);
		for (j := 0; j < len sensors; j++) {
			val := data[i][sensors[j]];
			if (val == -1)
				sys->fprint(fd, ",");
			else
				sys->fprint(fd, "%d,", val);
		}
		sys->fprint(fd, "\n");
		time += HOUR;
	}
	fd = nil;
	e := sh->run(ctxt, "bind" :: "-a" :: "#C" :: "/" :: nil);
	if (e != nil)
		sys->fprint(sys->fildes(2), "Could not bind #C: %s\n", e);
	else {
		e = sh->run(ctxt, "os" :: excelpath :: emuroot+filename :: nil);
		if (e != nil)
			sys->fprint(sys->fildes(2), "Could not start Excel: %s\n", e);
		else
			sys->remove(filename);
	}
}
