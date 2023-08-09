implement Dbmap;

include "sys.m";
	sys: Sys;
include "bufio.m";
	bufio: Bufio;
	Iobuf: import bufio;
include "draw.m";
	draw: Draw;
	Context, Display, Image, Font, Rect, Point, Chans: import draw;
include "arg.m";
include "daytime.m";
	daytime: Daytime;
include "rand.m";
	rand: Rand;
include "math.m";
	maths: Math;
include "math/geodesy.m";
	geodesy: Geodesy;
	OSGB36, WGS84, Natgrid: import Geodesy;
	str2en, en2os, str2lalo, lalo2str, en2lalo, lalo2en: import geodesy;
include "sh.m";
include "tk.m";
	tk: Tk;
	Toplevel: import tk;
include "tkclient.m";
	tkclient: Tkclient;
include "dialog.m";
	dialog: Dialog;

# no db	white		White
# < 35	light green	Palegreen
# < 40	green		Green
# < 45	dark green	Darkgreen
# < 50	yellow		Yellow
# < 55	ochre		Darkyellow
# < 60	orange		-
# < 65	cinnabar		Red
# < 70	carmine		-
# < 75	lilac red		-
# < 80	blue			Blue
# < 85	dark blue		Darkblue
# >= 85	black		Black

Rootdir: con "../";
Mapdir: con Rootdir + "york/";
Dbdir: con Rootdir + "db/";
Basef: con Rootdir + "york5000.bit";
Locfile: con Rootdir + "locations";
Sampler: con "resample";

Nscales: con 4;
Mod: con 1<<(Nscales-1);
Fact: con 4;

# these should be read in eventually

# map coordinate system
# TBS accurate coordinates needed for Mapr
Mapr: con Rect((459000, 450250), (461500, 452750));

Tilex: con (1451+Fact-1)/Fact;
Tiley: con (1726+Fact-1)/Fact;

Maxx: con Tilex;
Maxy: con Tiley;

# file format is
# 	starttime t db .. db t db .. db etc
Slen: con 4;	# no. bytes in a start time
Tlen: con 3;	# no. bytes in a time offset
Dlen: con 1;	# no. bytes in a decibel value
Ndb: con 2;	# no. decibel values following each time offset
Plen: con Tlen+Ndb*Dlen;	# no. bytes in a packet

Secdb: con 1;	# gap in seconds between db values
Nodb: con byte 0;
Nodb1: con byte 255;
Blank: con byte 254;
Border: con byte 255;

Limit: con 4;

Bit2byte: con 8;
Maxint: con 16r7fffffff;
Maxshort: con 1<<16;
Infinity: con Maxint;

Mind2: con 0;
Maxmem: con 4*2**20;

Alpha: con 128;

Cvt: con 0;
Eqsz: con 1;
Mapdist: con 1;
Mand: con 1;

Cache: adt{
	t: int;
	i: ref Image;
};

Map: adt{
	o: Point;
	logs: int;		# log2(scale)
	now: int;
	t: int;			# used counter
	mem: int;
	cache: array of array of Cache;
	ss: array of ref Sensor;
	vs: array of byte;
	zl: list of Point;
};

Screen: adt{
	ctxt: ref Context;
	t: ref Toplevel;
	d: ref Display;
	i: ref Image;
	w: int;
	h: int;
	cols: array of ref Image;
	bg: ref Image;
	black: ref Image;
	white: ref Image;
	font: ref Font;
};

Sensor: adt{
	id: string;	# an integer
	mp: Point;	# on map
	sp: Point;	# on screen
	t0: int;	# start time
	t1: int;	# end time
	db: array of byte;
};

screen: ref Screen;

Dbmap: module
{
	init: fn(nil: ref Context, argv: list of string);
};

init(ctxt: ref Context, args: list of string)
{
	sys = load Sys Sys->PATH;

	if(ctxt == nil)
		fatal("no context");

	bufio = load Bufio Bufio->PATH;
	draw = load Draw Draw->PATH;
	arg := load Arg Arg->PATH;
	daytime = load Daytime Daytime->PATH;
	rand = load Rand Rand->PATH;
	maths = load Math Math->PATH;
	geodesy = load Geodesy Geodesy->PATH;
	tk = load Tk Tk->PATH;
	tkclient = load Tkclient Tkclient->PATH;
	
	sys->pctl(sys->NEWPGRP, nil);
	geodesy->init(WGS84, Natgrid, -1);
	tkclient->init();
	rand->init(sys->millisec());
	arg->init(args);
	arg->setusage("dbmap [-rwxv] [file]");
	verbose := 0;
	mapfile := Mapdir + "0.0.0";

	while((o := arg->opt()) != 0)
		case(o){
		'r' =>
			readfiles(1);
			exit;
		'w' =>
			writefiles(random(2, 32), verbose);
			exit;
		'x' =>
			rmfiles();
			exit;
		'v' =>
			verbose = 1;
		'm' =>
			makemaps(ctxt, Sampler, Basef);
			exit;
		't' =>
			geodesy->format(OSGB36, Natgrid, -1);
			en := lalo2en(str2lalo("52:39:27.2531N 1:43:4.5177E").t1);
			sys->print("%f	%f\n", en.e, en.n);
			sys->print("%s\n", lalo2str(en2lalo((651409.903, 313177.270))));
			exit;
		* =>
			arg->usage();
		}
	args = arg->argv();
	if(args != nil){
		mapfile = hd args;
		args = tl args;
	}
	if(args != nil)
		arg->usage();
	arg = nil;

	(t, wmc) := tkclient->toplevel(ctxt, "", "Dbmap", Tkclient->Resize | Tkclient->Hide);
	cmdc := chan of string;
	tk->namechan(t, cmdc, "cmd");
	for(i := 0; i < len winconfig; i++)
		cmd(t, winconfig[i]);
	tkclient->onscreen(t, nil);
	tkclient->startinput(t, "kbd"::"ptr"::nil);
	fittoscreen(t);
	d := t.image.display;

	ioctl := chan of int;
	spawn tkinput(t, ioctl);
	<-ioctl;

	cols := initcols(d, Alpha);
	font := Font.open(d, "/fonts/lucidasans/euro.8.font");
	sn := ref Screen(ctxt, t, d, nil, 0, 0, cols, d.color(Draw->Grey), d.color(Draw->Black), d.color(Draw->White), font);
	(sn.w, sn.h, sn.i) = newimage(sn, ".p");
	screen = sn;

	ss := readfiles(verbose);
	readloc(Locfile, ss, verbose);
	map := initmap(ss);
	drawmap(sn, map, 0);
	map.vs = newvs(sn.w, sn.h);

	(t0, t1) := range(ss);
	if(Eqsz)
		map.now = 0;
	else
		map.now = t0;
	cmd(t, ".f0.s configure -from " + string t0 + " -to " + string (t1-1) + " -label {" + tim2str(t0) + "}");
	cmd(t, ".f0.s set " + string t0);
	cmd(t, "update");

	cs := chan of string;
	sync := chan of int;
	spawn interpolate(sn, map, cs, sync);
	pid := <-sync;

	but := 0;
	p := Point(-1, -1);
	dirty := 0;

	for(;;){
		alt{
		c := <-wmc =>
			case(c){
			"exit" =>
				killgrp(sys->pctl(0, nil));
				exit;
			* =>
				tkclient->wmctl(t, c);
				if(c[0] == '!'){
					img: ref Image;
					(sn.w, sn.h, img) = newimage(sn, ".p");
					if(img != sn.i){
						sn.i = img;
						kill(pid);
						drawmap(sn, map, 1);
						map.vs = newvs(sn.w, sn.h);
						spawn interpolate(sn, map, cs, sync);
						pid = <-sync;
					}
				}
			}
		c := <-cmdc =>
			(nil, lw) := sys->tokenize(c, " ");
			case(hd lw){
			"b11" =>
				p = Point(int hd tl lw, int hd tl tl lw);
				zoomin(sn, map, p);
				p = (-1, -1);
			"b22" =>
				;
			"b33" =>
				zoomout(sn, map);
			"b1" =>
				if(but&1)
					break;
				but |= 1;
				p = Point(int hd tl lw, int hd tl tl lw);
				showlabs(t, map, p, cs, c);
			"b2" =>
				if(but&2)
					break;
				but |= 2;
				drawmap(sn, map, 0);
				alt{
				cs <-= "c" =>
					ioctl <-= 0;
					<-cs;
					ioctl <-= 1;
				* =>
					;
				}
			"b3" =>
				if(but&4)
					break;
				but |= 4;
				q := Point(int hd tl lw, int hd tl tl lw);
				showlabs(t, map, q, cs, c);
			"b1r" =>
				but &= ~1;
				if(dirty){
					initvals(map.vs, sn.w, sn.h);
					dirty = 0;
				}
			"b2r" =>
				but &= ~2;
			"b3r" =>
				but &= ~4;
			"m" =>
				q := Point(int hd tl lw, int hd tl tl lw);
				if(but&1){
					if(p.x != q.x || p.y != q.y){
						map.o = map.o.sub(q.sub(p));
						drawmap(sn, map, 0);
						dirty = 1;
					}
					p = q;
				}
				showlabs(t, map, q, cs, c);
			"zi" =>
				zoomin(sn, map, p);
				p = (-1, -1);
			"zo" =>
				zoomout(sn, map);
			"db" =>
				drawmap(sn, map, 0);
				alt{
				cs <-= "c" =>
					ioctl <-= 0;
					<-cs;
					ioctl <-= 1;
				* =>
					;
				}
			"scale" =>
				tm := int hd tl lw;
				if(tm < t0)
					tm = t0;
				if(tm >= t1)
					tm = t1-1;
				if(Eqsz)
					map.now = tm-t0;
				else
					map.now = tm;
				cmd(t, ".f0.s configure -label {" + tim2str(tm) + "}");
				cmd(t, "update");
			"*" =>
				;
			}
		}
	}
}

waitc(map: ref Map, w: int, h: int, c: chan of string)
{
	for(;;){
		s := <-c;
		(nil, lw) := sys->tokenize(s, " ");
		case(hd lw){
		"c" =>
			return;
		"b1" or "b2" or "b3" or "m" =>
			x := int hd tl lw;
			y := int hd tl tl lw;
			if(x >= 0 && x < w && y >= 0 && y < h){
				if(Mand)
					db := map.vs[(h+2)*(x+1)+(y+1)];
				else
					db = map.vs[h*x+y];
				if(db == Blank || db == Border)
					db = Nodb;
				c <-= string db;
			}
			else
				c <-= string Nodb;
		}
	}
}

interpolate(sn: ref Screen, map: ref Map, c: chan of string, sync: chan of int)
{
	sync <-= sys->pctl(0, nil);
	for(;;){
		waitc(map, sn.w, sn.h, c);
		calculate(sn, map);
		drawclip(sn, map);
		reveal(sn.t);
		c <-= "c";
	}
}

newvs(w: int, h: int): array of byte
{
	if(Mand)
		vs := array[(w+2)*(h+2)] of byte;
	else
		vs = array[w*h] of byte;
	initvals(vs, w, h);
	return vs;
}

calculate(sn: ref Screen, map: ref Map)
{
	maxx := sn.w;
	maxy := sn.h;
	vs := map.vs;
	initvals(vs, maxx, maxy);
	if(Mand)
		setvals(sn, map, maxx, maxy);
	else{
		d := 0;
		for(x := 0; x < maxx; x++){
			for(y := 0; y < maxy; y++){
				v := pointval(map, x, y);
				vs[d++] = v;
				point(sn, (x, y), v);
			}
		}
	}
}

initvals(vs: array of byte, maxx: int, maxy: int)
{
	d: int;
	i: int;

	if(!Mand){
		n := maxx*maxy;
		for(i = 0; i < n; i++)
			vs[i] = Nodb;
		return;
	}
	supx := maxx+2;
	supy := maxy+2;
	n := supx*supy;
	for(i = 0; i < n; i++)
		vs[i] = Blank;
	d = 0;
	for(i = 0; i < supx; i++){
		vs[d] = Border;
		d += supy;
	}
	d = 0;
	for(i = 0; i < supy; i++){
		vs[d] = Border;
		d++;
	}
	d = n-1;
	for(i = 0; i < supx; i++){
		vs[d] = Border;
		d -= supy;
	}
	d = n-1;
	for(i = 0; i < supy; i++){
		vs[d] = Border;
		d--;
	}
}

setvals(sn: ref Screen, map: ref Map, maxx: int, maxy: int)
{
	edge: int;
	last := Blank;
	d := maxy + 3;
 	supy := maxy+2;
	vs := map.vs;
	for(x := 0; x < maxx; x++){
		for(y := 0; y < maxy; y++){
			col := vs[d];
			if(col == Blank){
				col = vs[d] = pointval(map, x, y);
				point(sn, (x, y), col);
				if(col == last)
					edge++;
				else{
					last = col;
					edge = 0;
				}
				if(edge >= Limit){
					crawlf(sn, map, x, y-edge, d-edge, last, supy);
					# prevent further crawlf()
					last = Blank;
				}
			}
			else{
				if(col == last)
					edge++;
				else{
					last = col;
					edge = 0;
				}
			}
			d++;
		}
		last = Blank;
		d += 2;
	}
}

crawlf(sn: ref Screen, map: ref Map, x, y, d: int, col: byte, supy: int)
{
	xinc, yinc, dxinc, dyinc: int;
	firstx, firsty: int;
	firstd: int;
	area := 0;
	count := 0;
 
	firstx = x;
	firsty = y;
	firstd = d;
	xinc = 1;
	dxinc = supy;
 
	# acw on success, cw on failure
	for(;;){
		if(getval(sn, map, x+xinc, y, d+dxinc) == col){
			x += xinc;
			d += dxinc;
			yinc = -xinc;
			dyinc = -dxinc;
			area += xinc*count;
			if(d == firstd)
				break;
		}else{ 
			yinc = xinc;
			dyinc = dxinc;
		}
		if(getval(sn, map, x, y+yinc, d+yinc) == col){
			y += yinc;
			d += yinc;
			xinc = yinc;
			dxinc = dyinc;
			count -= yinc;
			if(d == firstd)
				break;
		}else{ 
			xinc = -yinc;
			dxinc = -dyinc;
		}
	}
	if(area > 0)	# cw
		crawlt(sn, map, firstx, firsty, firstd, col, supy);
}

crawlt(sn: ref Screen, map: ref Map, x, y, d: int, col: byte, supy: int)
{
	yinc, dyinc: int;
 
	firstd := d;
	xinc := 1;
	dxinc := supy;
 	vs := map.vs;
	for(;;){
		if(getval(sn, map, x+xinc, y, d+dxinc) == col){
			x += xinc;
			d += dxinc;
			yinc = -xinc;
			dyinc = -dxinc;
			if(vs[d+dxinc] == Blank)
				fillline(sn, map, x+xinc, y, d+dxinc, yinc, dyinc, col);
			if(d == firstd)
				break;
		}
		else{ 
			yinc = xinc;
			dyinc = dxinc;
		}
		if(getval(sn, map, x, y+yinc, d+yinc) == col){
			y += yinc;
			d += yinc;
			xinc = yinc;
			dxinc = dyinc;
			if(vs[d-dxinc] == Blank)
				fillline(sn, map, x-xinc, y, d-dxinc, yinc, dyinc, col);
			if(d == firstd)
				break;
		}
		else{ 
			xinc = -yinc;
			dxinc = -dyinc;
		}
	}
}

fillline(sn: ref Screen, map: ref Map, x, y, d, dir, dird: int, col: byte)
{
	x0 := x;
	vs := map.vs;
	while(vs[d] == Blank){
		vs[d] = col;
		x -= dir;
		d -= dird;
	}
	if(pointval(map, (x0+x+dir)/2, y) != col){
		# island - undo colouring
		do{
			d += dird;
			x += dir;
			vs[d] = pointval(map, x, y);
			point(sn, (x, y), vs[d]);
		}while (x != x0);
		return;
	}
	horline(sn, x0, x, y, col);
}

horline(sn: ref Screen, x0, x1, y: int, col: byte)
{
	img := sn.i;
	if(x0 < x1)
		r := Rect((x0, y), (x1, y+1));
	else
		r = Rect((x1+1, y), (x0+1, y+1));
	img.draw(r.addpt(img.r.min), colour(col, sn.cols), nil, (0, 0));
}

pointval(map: ref Map, i, j: int): byte
{
	if(Mapdist)
		(x, y) := pix2en(map, (i, j));
	else
		(x, y) = (i, j);
	return byte idinterp((x, y), map.ss, map.now);
}

getval(sn: ref Screen, map: ref Map, x, y, d: int): byte
{
	vs := map.vs;
	if(vs[d] == Blank){
		vs[d] = pointval(map, x, y);
		point(sn, (x, y), vs[d]);
	}
	return vs[d];
}

point(sn: ref Screen, p: Point, col: byte)
{
	img := sn.i;
	img.draw(Rect(p, p.add((1,1))), colour(col, sn.cols), nil, (0,0));
}

# inverse distance interpolation
idinterp(p: Point, ss: array of ref Sensor, t: int): real
{
	n := len ss;
	sv := sd := 0.0;
	for(i := 0; i < n; i++){
		s := ss[i];
		if(Mapdist)
			q := s.mp;
		else
			q = s.sp;
		if(Eqsz)
			v := real s.db[t];
		else
			v = real dbval(s, t);
		if(v == real Nodb)
			continue;
		if(Cvt)
			v = maths->pow(10.0, v/10.0);
		d2 := (q.x-p.x)**2+(q.y-p.y)**2;
		if(d2 <= Mind2)
			return v;
		id2 := 1.0/real d2;
		sv += id2*v;
		sd += id2;
	}
	if(sd == 0.0)
		return 0.0;
	if(Cvt)
		return 10.0*maths->log10(sv/sd);
	return sv/sd;
}

dbval(s: ref Sensor, t: int): byte
{
	if(t < s.t0 || t >= s.t1)
		return Nodb;
	return s.db[t-s.t0];
}

range(ss: array of ref Sensor): (int, int)
{
	t0 := Maxint;
	t1 := 0;
	n := len ss;
	for(i := 0 ; i < n; i++){
		if(ss[i].t0 < t0)
			t0 = ss[i].t0;
		if(ss[i].t1 > t1)
			t1 = ss[i].t1;
	}
	return (t0, t1);
}

writefiles(n: int, verbose: int)
{
	b := array[4] of byte;
	for(i := 0; i < n; i++){
		fd := sys->create(string i + ".db", Sys->OWRITE, 8r644);
		r := random(1024, 1084);
		sys->write(fd, leput4(b, r), 4);
		if(verbose)
			sys->print("%d/%d	", i, r);
		j0 := -Ndb;
		for(j := 0; j < 128; j++){
			if(j >= j0+Ndb && random(0, 2)){
				j0 = j;
				sys->write(fd, leput3(b, j), 3);
				if(verbose)
					sys->print("(%d ", j);
				for(k := 0; k < Ndb; k++){
					r = random(30, 90);
					sys->write(fd, leput1(b, r), 1);
					if(verbose)
						sys->print("%d ", r);
				}
				if(verbose)
					sys->print(")");
			}
		}
		if(verbose)
			sys->print("\n");
	}
}

readfiles(verbose: int): array of ref Sensor
{
	ss := reads(dbfiles());
	if(verbose){
		n := len ss;
		(t0, t1) := range(ss);
		sys->print("id	");
		for(i := 0; i < n; i++)
			sys->print("%s	", ss[i].id);
		sys->print("\n");
		sys->print("t0	");
		for(i = 0; i < n; i++)
			sys->print("%d	", ss[i].t0*Secdb);
		sys->print("\n");
		sys->print("t1	");
		for(i = 0; i < n; i++)
			sys->print("%d	", ss[i].t1*Secdb);
		sys->print("\n");
		for(t := t0; t < t1; t++){
			sys->print("%d	", t*Secdb);
			for(i = 0; i < n; i++)
				sys->print("%d ", int dbval(ss[i], t));
			sys->print("\n");
		}
	}
	return ss;
}

rmfiles()
{
	for(ls := dbfiles(); ls != nil; ls = tl ls)
		sys->remove(hd ls);
}

reads(ls: list of string): array of ref Sensor
{
	n := len ls;
	ss := array[n] of ref Sensor;
	for(k := 0 ; ls != nil; ls = tl ls){
		s := read(hd ls);
		if(s != nil)
			ss[k++] = s;
	}
	if(k < n)
		ss = ss[0: k];
	if(Eqsz){
		n = len ss;
		(t0, t1) := range(ss);
		for(k = 0; k < n; k++){
			s := ss[k];
			db := array[t1-t0] of { * => Nodb };
			db[s.t0-t0: ] = s.db[0: s.t1-s.t0];
			s.db = db;
			s.t0 = t0;
			s.t1 = t1;
		}
	}
	return ss;
}

read(f: string): ref Sensor
{
	fd := sys->open(f, Sys->OREAD);
	if(fd == nil){
		error(sys->sprint("%s: cannot open", f));
		return nil;
	}
	b := array[4] of byte;
	if(sys->read(fd, b, Slen) < Slen){
		error(sys->sprint("%s: too small", f));
		return nil;
	}
	s := ref Sensor;
	s.mp = (Infinity, Infinity);
	s.id = prefix(f);
	s.t0 = divr(leget4(b, 0), Secdb);
	(ok, d) := sys->fstat(fd);
	if(ok < 0){
		error(sys->sprint("%s: cannot stat", f));
		return nil;
	}
	if(d.length > big Maxint){
		error(sys->sprint("%s: too big", f));
		return nil;
	}
	readdb(s, fd, int d.length);
	return s;
}

readdb(s: ref Sensor, fd: ref Sys->FD, leng: int)
{
	b := array[Sys->ATOMICIO] of byte;
	n := 3*Ndb*((leng-Slen)/Plen)/2;
	db := array[n] of { * => Nodb };
	adj := 0;
	mt := lt := at := 0;
	for(;;){
		m := sys->read(fd, b, (Sys->ATOMICIO/Plen)*Plen);
		if(m <= 0)
			break;
		if(m%Plen != 0)
			fatal(sys->sprint("bad read %d", m));
		m /= Plen;
		o := 0;
		for(i := 0; i < m; i++){
			t := divr(leget3(b, o), Secdb);
			o += Tlen;
			if(lt-t > Maxshort/2 && lt-at > Maxshort/2){	# 16 bit overflow
				adj += Maxshort;
				at = t+adj;
			}
			t += adj;
			if(t > mt)
				mt = t;
			lt = t;
			k := t;
			for(j := 0; j < Ndb; j++){
				v := leget1(b, o++);
				if(v == Nodb1)
					v = Nodb;
				if(k >= n){
					db = grow(db);
					n = len db;
				}
				db[k++] = v;
			}
		}
	}
	mt += Ndb;
	s.t1 = s.t0+mt;
	if(!Eqsz && n > mt)
		db = contract(db, mt);
	s.db = db;
}

readloc(f: string, ss: array of ref Sensor, verbose: int)
{
	b := bufio->open(f, Sys->OREAD);
	if(b == nil){
		error(sys->sprint("%s: cannot open", f));
		return;
	}
	while((s := b.gets('\n')) != nil){
		if(s == nil || s[0] == '#')
			continue;
		(n, ls) := sys->tokenize(s, " \t\r\n");
		if(n == 0)
			continue;
		if(n != 2 && n != 3){
			error(sys->sprint("bad location entry %s", s));
			continue;
		}
		id := int hd ls;
		s = hd tl ls;
		if(n == 3)
			s += " " + hd tl tl ls;
		(ok, en) := str2en(s);
		if(!ok){
			error(sys->sprint("bad map reference %s", s));
			continue;
		}
		if(verbose){
			if(s[len s-1] == '\n')
				s = s[0: len s-1];
			lalo := en2lalo(en);
			sys->print("%s	%s	%s\n", s, en2os(en), lalo2str(lalo));
		}
		se := lookup(string id, ss);
		if(se != nil){
			if(se.mp.x != Infinity || se.mp.y != Infinity)
				error(sys->sprint("id %d has duplicate locations", id));
			se.mp = (int en.e, int en.n);
		}
		else
			error(sys->sprint("id %d unknown", id));
	}
	n := len ss;
	for(i := 0; i < n; i++){
		se := ss[i];
		mp := se.mp;
		if(mp.x == Infinity && mp.y == Infinity){
			error(sys->sprint("id %s has no location", se.id));
			se.mp = randp(Mapr);
		}
	}		
}

lookup(id: string, ss: array of ref Sensor): ref Sensor
{
	n := len ss;
	for(i := 0; i < n; i++)
		if(ss[i].id == id)
			return ss[i];
	return nil;
}

makemaps(ctxt: ref Context, cmd: string, basef: string)
{
	fd := openwait(sys->pctl(0, nil));
	for(i := 0; i < Nscales; i++){
		s := 1<<i;
		for(x := 0; x < s; x++){
			for(y := 0; y < s; y++){
				argl := basef :: nil;
				argl = "-f" + sys->sprint("%d.%d.%d", i, x, y) :: argl;
				argl = "-m" + string Mod :: argl;
				argl = "-X" + string x :: argl;
				argl = "-Y" + string y :: argl;
				argl = "-w" + string s + "f" :: argl;
				argl = "-h" + string s + "f" :: argl;
				if(s <= Fact){
					argl = "-x" + string(Fact/s) + "f" :: argl;
					argl = "-y" + string(Fact/s) + "f" :: argl;
				}
				else{
					argl = "-x" + string(100*s/Fact) + "%" :: argl;
					argl = "-y" + string(100*s/Fact) + "%" :: argl;
				}
				argl = cmd :: argl;
				sync := chan of int;
				spawn exec(ctxt, argl, sync);
				pid := <-sync;
				wait(fd, pid);
			}
		}
	}
}

initmap(ss: array of ref Sensor): ref Map
{
	map := ref Map((0, 0), 0, 0, 0, 0, nil, ss, nil, nil);
	map.cache = array[Nscales] of array of Cache;
	for(i := 0; i < Nscales; i++){
		s := 1<<i;
		map.cache[i] = array[s*s] of Cache;
		for(j := 0; j < s*s; j++)
			map.cache[i][j].t = 0;
	}
	return map;
}

drawmap(sn: ref Screen, map: ref Map, resz: int)
{
	img := sn.i;
	(w, h) := (sn.w, sn.h);
	s := 1<<map.logs;
	o := map.o;
	sr := Rect(o, (o.x+w, o.y+h));
	mr := Rect((0, 0), (s*Tilex, s*Tiley));
	(ir, c) := sr.clip(mr);
	if(c == 0 || !ir.eq(sr) || resz)
		img.draw(img.r, sn.bg, nil, (0, 0));
	if(c == 0){
		reveal(sn.t);
		return;
	}
	for(i := ir.min.x/Tilex; i <= (ir.max.x-1)/Tilex; i++){
		for(j := ir.min.y/Tiley; j <= (ir.max.y-1)/Tiley; j++){
			tr := Rect((i*Tilex, j*Tiley), ((i+1)*Tilex, (j+1)*Tiley));
			(r, nil) := ir.clip(tr);
			img.draw(r.subpt(o), getimg(sn.d, map, i, j), nil, r.min.sub(tr.min));
		}
	}
	ss := map.ss;
	n := len ss;
	for(i = 0; i < n; i++){
		se := ss[i];
		p := se.sp = en2pix(map, se.mp);
		if(p.in(img.r)){
			f := 256/s;
			if(map.logs == Nscales-1){
				bx := sn.font.width(se.id);
				by := sn.font.height;
				img.text((p.x-bx/2, p.y-h/f-by), sn.black, (0, 0), sn.font, se.id);
			}
			plotr(img, Rect((p.x-w/f, p.y-h/f), (p.x+w/f, p.y+h/f)), sn.black);
		}
	}
	reveal(sn.t);
}

drawclip(sn: ref Screen, map: ref Map)
{
	img := sn.i;
	(w, h) := (sn.w, sn.h);
	s := 1<<map.logs;
	o := map.o;
	sr := Rect(o, (o.x+w, o.y+h));
	mr := Rect((0, 0), (s*Tilex, s*Tiley));
	(ir, c) := sr.clip(mr);
	if(c == 0)
		img.draw(img.r, sn.bg, nil, (0, 0));
	else if(!ir.eq(sr)){
		sr = sr.subpt(o);
		ir = ir.subpt(o);
		for(lr := rsubr(sr, ir); lr != nil; lr = tl lr)
			img.draw(hd lr, sn.bg, nil, (0, 0));
	}
}

getimg(d: ref Display, map: ref Map, x: int, y: int): ref Image
{
	ls := map.logs;
	cache := map.cache[ls];
	ix := (1<<ls)*x+y;
	if((i := cache[ix].i) == nil){
		i = cache[ix].i = d.open(sys->sprint("%s%d.%d.%d", Mapdir, ls, x, y));
		if(i != nil){
			cache[ix].t = ++map.t;
			map.mem += i.depth*i.r.dx()*i.r.dy()/Bit2byte;
			while(map.mem > Maxmem){
				(ls, ix) = lru(map);
				im := map.cache[ls][ix].i;
				map.mem -= im.depth*im.r.dx()*im.r.dy()/Bit2byte;
				im = map.cache[ls][ix].i = nil;
			}
		}
	}
	else
		cache[ix].t = ++map.t;
	return i;
}

lru(map: ref Map): (int, int)
{
	min := Maxint;
	ls := 0;
	ix := 0;
	for(i := 0; i < Nscales; i++){
		s := 1<<i;
		cache := map.cache[i];
		for(j := 0; j < s*s; j++){
			if(cache[j].i != nil && cache[j].t < min){
				min = cache[j].t;
				ls = i;
				ix = j;
			}
		}
	}
	return (ls, ix);
}

zoomin(sn: ref Screen, map: ref Map, p: Point)
{
	if(map.logs < Nscales-1){
		map.zl = map.o :: map.zl;
		c := (sn.w/2, sn.h/2);
		if(p.x < 0 || p.y < 0)
			p = c;
		map.o = map.o.add(p).mul(2).sub(c);
		map.logs++;
		drawmap(sn, map, 0);
		initvals(map.vs, sn.w, sn.h);
	}
}

zoomout(sn: ref Screen, map: ref Map)
{
	if(map.logs > 0){
		map.o = hd map.zl;
		map.zl = tl map.zl;
		map.logs--;
		drawmap(sn, map, 0);
		initvals(map.vs, sn.w, sn.h);
	}
}

pix2en(map: ref Map, p: Point): Point
{
	s := real(1<<map.logs);
	(w, h) := (real Mapr.dx(), real Mapr.dy());
	x := (real(map.o.x+p.x))*w/(s*real Tilex)+real Mapr.min.x;
	y := h-(real(map.o.y+p.y))*h/(s*real Tiley)+real Mapr.min.y;
	return (int x, int y);
}

en2pix(map: ref Map, p: Point): Point
{
	s := 1<<map.logs;
	(w, h) := (Mapr.dx(), Mapr.dy());
	x := divr((p.x-Mapr.min.x)*s*Tilex, w)-map.o.x;
	y := divr((h-p.y+Mapr.min.y)*s*Tiley, h)-map.o.y;
	return (x, y);
}

showlabs(t: ref Toplevel, map: ref Map, p: Point, cs: chan of string, c: string)
{
	showlalo(t, map, p);
	showos(t, map, p);
	# showen(t, map, p);
	# showpix(t, p);
	showdb(t, cs, c);
}

showlalo(t: ref Toplevel, map: ref Map, p: Point)
{
	en := pix2en(map, p);
	lalo := en2lalo((real en.x, real en.y));
	cmd(t, ".f1.ll configure -text {" + lalo2str(lalo) + "}");
}

showos(t: ref Toplevel, map: ref Map, p: Point)
{
	en := pix2en(map, p);
	cmd(t, ".f1.os configure -text {" + en2os((real en.x, real en.y)) + "}");
}

showen(t: ref Toplevel, map: ref Map, p: Point)
{
	en := pix2en(map, p);
	cmd(t, ".f1.en configure -text " + sys->sprint("{(%d, %d)}", en.x, en.y));
}

showpix(t: ref Toplevel, p: Point)
{
	cmd(t, ".f1.pix configure -text " + sys->sprint("{(%d, %d)}", p.x, p.y));
}

showdb(t: ref Toplevel, cs: chan of string, c: string)
{
	alt{
	cs <-= c =>
		cmd(t, ".f2.db configure -text {" + <-cs + " Db}");
	* =>
		;
	}
}

rect(x0, x1, y0, y1: int): Rect
{
	return ((x0, y0), (x1, y1));
}

rsubr(r: Rect, s: Rect): list of Rect
{
	lr: list of Rect;

	# s in r
	if(s.max.y < r.max.y){
		lr = rect(r.min.x, r.max.x, s.max.y, r.max.y) :: lr;
		r.max.y = s.max.y;
	}
	if(s.min.y > r.min.y){
		lr = rect(r.min.x, r.max.x, r.min.y, s.min.y) :: lr;
		r.min.y = s.min.y;
	}
	if(s.max.x < r.max.x){
		lr = rect(s.max.x, r.max.x, r.min.y, r.max.y) :: lr;
		r.max.x = s.max.x;
	}
	if(s.min.x > r.min.x){
		lr = rect(r.min.x, s.min.x, r.min.y, r.max.y) :: lr;
		r.min.x = s.min.x;
	}
	if(!r.eq(s))
		fatal("rsubr");
	return lr;
}

# resize(i: ref Image, nr: Rect, d: ref Display): ref Image
# {
# 	r := i.r;
# 	b := readpix(i, r);
# 	nb := resample(b, r.dx(), r.dy(), nr.dx(), nr.dy());
# 	b = nil;
# 	ni := writepix(nb, nr, i.chans, d);
# 	nb = nil;
# 	return ni;
# }

# resample(src: array of byte, sw, sh: int, dw, dh: int): array of byte
# {
# 	if(src == nil || sw == 0 || sh == 0 || dw == 0 || dh == 0)
# 		return src;
# 	xf := real sw/real dw;
# 	yf := real sh/real dh;
# 	dst := array[dw*dh] of byte;
# 	sindx := array[dw] of int;
# 	dx := 0.0;
# 	for(x := 0; x < dw; x++){
# 		sx := int dx;
# 		dx += xf;
# 		if(sx >= sw)
# 			sx = sw-1;
# 		sindx[x] = sx;
# 	}
# 	dy := 0.0;
# 	di := 0;
# 	for(y := 0; y < dh; y++){
# 		sy := int dy;
# 		dy += yf;
# 		if(sy >= sh)
# 			sy = sh-1;
# 		soff := sy*sw;
# 		for(x = 0; x < dw; x++)
# 			dst[di++] = src[soff+sindx[x]];
# 	}
# 	sindx = nil;
# 	return dst;
# }

# assumes depth=8
# readpix(i: ref Image, r: Rect): array of byte
# {
# 	n := r.dx()*r.dy();
# 	b := array[n] of byte;
# 	if(i.readpixels(r, b) != n)
# 		fatal("readpixels");
# 	return b;
# }

# assumes depth=8
# writepix(b: array of byte, r: Rect, c: Chans, d: ref Display): ref Image
# {
# 	n := r.dx()*r.dy();
# 	i := d.newimage(r, c, 0, 0);
# 	if(i.writepixels(r, b) != n)
# 		fatal("writepixels");
# 	return i;
# }

exec(ctxt: ref Context, argl: list of string, sync: chan of int)
{
	sync <-= sys->pctl(0, nil);
	f := hd argl;
	if(len f < 4 || f[len f-4:] != ".dis")
		f += ".dis";
	c := load Command f;
	if(c == nil){
		c = load Command "/dis/" + f;
		if(c == nil)
			fatal(sys->sprint("%s does not exist", hd argl));
	}
	for(l := argl; l != nil; l = tl l)
		sys->print("%s ", hd l);
	sys->print("\n");
	c->init(ctxt, argl);
}

openwait(pid: int): ref Sys->FD
{
	w := sys->sprint("#p/%d/wait", pid);
	fd := sys->open(w, Sys->OREAD);
	if(fd == nil)
		fatal("fd == nil in wait");
	return fd;
}

wait(fd: ref Sys->FD, pid: int)
{
	n: int;

	buf := array[Sys->WAITLEN] of byte;
	status := "";
	for(;;){
		if((n = sys->read(fd, buf, len buf)) < 0)
			fatal("bad read in wait");
		status = string buf[0:n];
		break;
	}
	if(int status != pid)
		fatal("bad status in wait");
	if(status[len status - 1] != ':')
		fatal(sys->sprint("%s", status));
}

grow(a: array of byte): array of byte
{
	n := len a;
	b := array[3*n/2] of { * => Nodb };
	b[0: ] = a[0: n];
	return b;
}

contract(a: array of byte, n: int): array of byte
{
	b := array[n] of byte;
	b[0: ] = a[0: n];
	return b;
}

divr(v: int, m: int): int
{
	return (v+m/2)/m;
}

prefix(s: string): string
{
	n := len s;
	for(i := n-1; i >= 0; i--)
		if(s[i] == '/'){
			s = s[i+1: ];
			break;
		}
	n = len s;
	for(i = 0; i < n; i++)
		if(s[i] == '.'){
			s = s[0: i];
			break;
		}
	return s;
}

leget1(b: array of byte, n: int): byte
{
	return b[n];
}

leget2(b: array of byte, n: int): int
{
	return int b[n] | int b[n+1]<<8;
}

leget3(b: array of byte, n: int): int
{
	return int b[n] | int b[n+1]<<8 | int b[n+2]<<16;
}

leget4(b: array of byte, n: int): int
{
	return int b[n] | int b[n+1]<<8 | int b[n+2]<<16 | int b[n+3]<<24;
}

leput1(b: array of byte, v: int): array of byte
{
	b[0] = byte v;
	return b;
}

leput2(b: array of byte, v: int): array of byte
{
	b[0] = byte v;
	b[1] = byte (v>>8);
	return b;
}

leput3(b: array of byte, v: int): array of byte
{
	b[0] = byte v;
	b[1] = byte (v>>8);
	b[2] = byte (v>>16);
	return b;
}

leput4(b: array of byte, v: int): array of byte
{
	b[0] = byte v;
	b[1] = byte (v>>8);
	b[2] = byte (v>>16);
	b[3] = byte (v>>24);
	return b;
}

initcols(d: ref Display, alpha: int): array of ref Image
{
	vals := array[] of {
		Draw->White, Draw->Palegreen, Draw->Green, Draw->Darkgreen,
		Draw->Yellow, rgb(255, 213, 0), rgb(255, 170, 0), Draw->Red,
		rgb(204, 0, 136), rgb(204, 0, 85), Draw->Blue, Draw->Darkblue,
		Draw->Black,
	};
	n := len vals;
	cols := array[n] of ref Image;
	for(i := 0; i < n; i++)
		cols[i] = setalpha(d, vals[i], alpha);
	return cols;
}

colour(b: byte, cols: array of ref Image): ref Image
{
	c := int b;
	if(c == 0)
		ci := 0;
	else if(c < 35)
		ci = 1;
	else if(c >= 85)
		ci = 12;
	else
		ci = (c-35)/5+2;
	return cols[ci];
}

dbfiles(): list of string
{
	ls: list of string;

	for(i := 50; i >= 0; i--){
		s := Dbdir + string i + ".db";
		if(sys->open(s, Sys->OREAD) != nil)
			ls = s :: ls;
	}
	return ls;
}

plot(i: ref Image, p: Point, c: byte, cols: array of ref Image)
{
	i.draw(Rect((p.x, p.y), (p.x+1, p.y+1)), colour(c, cols), nil, (0, 0));
}

plotc(i: ref Image, p: Point, c: byte, cols: array of ref Image)
{
	i.fillellipse(p, 10, 10, colour(c, cols), (0, 0));
}

plotr(i: ref Image, r: Rect, c: ref Image)
{
	i.draw(r, c, nil, (0, 0));
}

tim2str(t: int): string
{
	return daytime->text(daytime->local(t));
}

random(a: int, b: int): int
{
	return rand->rand(b-a+1)+a;
}

randp(r: Rect): Point
{
	return (random(0, r.dx()-1)+r.min.x, random(0, r.dy()-1)+r.min.y);
}

rgb(r: int, g: int, b: int): int
{
	return r<<24|g<<16|b<<8|16rff;
}

setalpha(d: ref Display, c: int, a: int): ref Image
{
	c = draw->setalpha(c, a);
	return d.newimage(((0, 0), (1, 1)), Draw->RGBA32, 1, c);
}

reveal(t: ref Toplevel)
{
	cmd(t, ".p dirty; update");
}

newimage(sn: ref Screen, p: string): (int, int, ref Image)
{
	t := sn.t;
	i := sn.i;
	wh := Point(int cmd(t, p + " cget -actwidth"), int cmd(t, p + " cget -actheight"));
	if(i == nil || wh.x != i.r.max.x || wh.y != i.r.max.y){
		i = t.image.display.newimage(((0, 0), wh), Draw->RGBA32, 0, Draw->Black);
		if(i == nil){
			screen = nil;
			fatal("no screen");
		}
		tk->putimage(t, p, i, nil);
		i.draw(i.r, sn.white, nil, (0, 0));
	}
	return (wh.x, wh.y, i);
}

tkinput(t: ref Toplevel, ctl: chan of int)
{
	ctl <-= 0;
	on := 1;
	for(;;){
		alt{
		on = <-ctl =>
			;
		c := <-t.ctxt.ctl or
		c = <-t.wreq =>
			tkclient->wmctl(t, c);
		c := <-t.ctxt.kbd =>
			if(on)
				tk->keyboard(t, c);
		c := <-t.ctxt.ptr =>
			if(on)
				tk->pointer(t, *c);
		}
	}
}

cmd(t: ref Toplevel, s: string): string
{
	e := tk->cmd(t, s);
	if(e != nil && e[0] == '!')
		fatal(sys->sprint("tk error on '%s': %s", s, e));
	return e;
}

fittoscreen(t: ref Toplevel)
{
	actr: Rect;

	if(t.image == nil || t.image.screen == nil)
		return;
	r := t.image.screen.image.r;
	scrsize := r.max.sub(r.min);
	bd := 2*int cmd(t, ". cget -bd");
	winsize := Point(int cmd(t, ". cget -actwidth")+bd, int cmd(t, ". cget -actheight")+bd);
	if(winsize.x > scrsize.x)
		cmd(t, ". configure -width " + string (scrsize.x-bd));
	if(winsize.y > scrsize.y)
		cmd(t, ". configure -height " + string (scrsize.y-bd));
	actr.min = Point(int cmd(t, ". cget -actx"), int cmd(t, ". cget -acty"));
	actr.max = actr.min.add((int cmd(t, ". cget -actwidth")+bd, int cmd(t, ". cget -actheight")+bd));
	(dx, dy) := (actr.dx(), actr.dy());
	if(actr.max.x > r.max.x)
		(actr.min.x, actr.max.x) = (r.max.x-dx, r.max.x);
	if(actr.max.y > r.max.y)
		(actr.min.y, actr.max.y) = (r.max.y-dy, r.max.y);
	if(actr.min.x < r.min.x)
		(actr.min.x, actr.max.x) = (r.min.x, r.min.x+dx);
	if(actr.min.y < r.min.y)
		(actr.min.y, actr.max.y) = (r.min.y, r.min.y+dy);
	cmd(t, ". configure -x " + string actr.min.x + " -y " + string actr.min.y);
}

kill(pid: int): int
{
	if(pid < 0)
		return 0;
	fd := sys->open("#p/"+string pid+"/ctl", Sys->OWRITE);
	if(fd == nil)
		return -1;
	if(sys->write(fd, array of byte "kill", 4) != 4)
		return -1;
	return 0;
}

killgrp(pid: int): int
{
	if(pid < 0)
		return 0;
	fd := sys->open("#p/"+string pid+"/ctl", Sys->OWRITE);
	if(fd == nil)
		return -1;
	if(sys->write(fd, array of byte "killgrp", 7) != 7)
		return -1;
	return 0;
}

error(e: string)
{
	if(dialog == nil){
		dialog = load Dialog Dialog->PATH;
		dialog->init();
	}
	sys->fprint(sys->fildes(2), "%s\n", e);
	if(screen != nil)
		dialog->prompt(screen.ctxt, screen.t.image, nil, "Error", e, 0, "ok" :: nil);
}

fatal(e: string)
{
	error(e);
	killgrp(sys->pctl(0, nil));
	exit;
}

winconfig := array[] of {
	"frame .f0",
	"scale .f0.s -orient horizontal -showvalue 0 -label {1970} -command {send cmd scale}",
	"frame .f",
	"button .f.zi -text {zoomin} -command {send cmd zi}",
	"button .f.zo -text {zoomout} -command {send cmd zo}",
	"button .f.db -text {dbmap} -command {send cmd db}",
	"frame .f1",
	"label .f1.pix -anchor w -text {(0, 0)}",
	"label .f1.en -anchor w -text {(0, 0)}",
	"label .f1.os -anchor w -text {SV0000000000}",
	"label .f1.ll -anchor w -text {00:00:00.00N 00:00:00.00E}",
	"frame .f2",
	"label .f2.db -anchor center -text {0 Db}",
	"panel .p -width " + string Maxx + " -height " + string Maxy + " -bd 2 -relief flat",

	"pack .f0.s -side top -fill x",

	"pack .f.zi -side left",
	"pack .f.zo -side right",
	"pack .f.db -side top",

	"pack .f1.ll -side left -fill x",
	"pack .f1.os -side right -fill x",
	# "pack .f1.pix -side top -fill x",
	# "pack .f1.en -side top -fill x",

	"pack .f2.db -side top -fill x",
	
	"pack .f0 -side top -fill x",
	"pack .f1 -side top -fill x",
	"pack .f -side top -fill x",
	"pack .f2 -side top -fill x",

	"pack .p -side bottom -fill both -expand 1",
	"pack propagate . 0",
	"bind .p <Double-Button-1> { send cmd b11 %x %y %b}",
	"bind .p <Double-Button-2> { send cmd b22 %x %y %b}",
	"bind .p <Double-Button-3> { send cmd b33 %x %y %b}",
	"bind .p <Button-1> { send cmd b1 %x %y %b}",
	"bind .p <Button-2> { send cmd b2 %x %y}",
	"bind .p <Button-3> { send cmd b3 %x %y}",
	"bind .p <ButtonRelease-1> { send cmd b1r %x %y}",
	"bind .p <ButtonRelease-2> { send cmd b2r %x %y}",
	"bind .p <ButtonRelease-3> { send cmd b3r %x %y}",
	"bind .p <Motion> { send cmd m %x %y}",
	"update"
};
