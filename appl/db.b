implement Db;

include "sys.m";
	sys: Sys;
include "draw.m";
	draw: Draw;
	Display, Image, Font, Rect, Point, Endsquare: import draw;
include "arg.m";
include "rand.m";
	rand: Rand;
include "tk.m";
	tk: Tk;
	Toplevel: import tk;
include "tkclient.m";
	tkclient: Tkclient;

PWIDTH: con 10;
MINDB: con 40;
MAXDB: con 100;
LOGMINSCALE: con 0;
LOGMAXSCALE: con 15;
LOGSCALE: con 7;
NDBS: con 512;
MAXXH: con NDBS;
MAXYH: con MAXXH/4;
NDB: con 2;

Db: module
{
	init: fn(nil: ref Draw->Context, nil: list of string);
};

newimage(win: ref Toplevel, p: string): (ref Image, int, int)
{
	maxx := int cmd(win, p + " cget -actwidth");
	maxy := int cmd(win, p + " cget -actheight");
	img := win.image.display.newimage(((0, 0), (maxx, maxy)), win.image.chans, 0, Draw->Black);
	tk->putimage(win, p, img, nil);
	img.draw(img.r, win.image.display.rgb(0, 0, 0), nil, (0, 0));
	reveal(win, p);
	return (img, maxx, maxy);
}

reveal(win: ref Toplevel, p: string)
{
	cmd(win, p + " dirty; update");
}

init(ctxt: ref Draw->Context, args: list of string)
{
	sys = load Sys Sys->PATH;
	draw = load Draw Draw->PATH;
	tk = load Tk Tk->PATH;
	tkclient = load Tkclient Tkclient->PATH;
	rand = load Rand Rand->PATH;
	arg := load Arg Arg->PATH;

	if(sys->bind("#t", "/dev", Sys->MAFTER) < 0)
		;	# fatal("cannot bind serial device");
	rand->init(sys->millisec());
	arg->init(args);
	arg->setusage("serial [-d serialdev]");
	s := "/dev/eia0";
	while((o := arg->opt()) != 0)
		case(o){
		'd' =>
			s = arg->earg();
			if(len s == 1 && s[0] >= '0' && s[0] <= '9')
				s = "/dev/eia" + s;
			else if(len s == 4 && s[0: 3] == "eia")
				s = "/dev/" + s;
		* =>
			arg->usage();
		}
	if(arg->argv() != nil)
		arg->usage();
	arg = nil;

	sfd := sys->open(s, Sys->ORDWR);
	if(sfd == nil)
		exit;
	cfd := sys->open(s+"ctl", Sys->ORDWR);
	sys->fprint(cfd, "f");
	sys->fprint(cfd, "b57600");
	sys->fprint(cfd, "i8");

	tkclient->init();
	
	pidr := -1;
	cr := chan of (int, int, int);
	syncr := chan of int;
	spawn reader(sfd, cr, syncr);
	pidr = <-syncr;

	exitc := chan of int;	# should be one per process

	lids: list of (int, chan of int, int);

	for(;;){
		alt{
		(id, nil, db) := <-cr =>
			c: chan of int;

			for(l := lids; l != nil; l = tl l)
				if((hd l).t0 == id)
					break;
			if(l == nil){
				pid := -1;
				c = chan of int;
				sync := chan of int;
				spawn history(ctxt, string id, c, sync, exitc);
				pid = <-sync;
				lids = (id, c, pid) :: lids;
			}
			else
				c = (hd l).t1;
			if(c != nil)
				c <-= db;	# time stamp ignored for now
		<-exitc =>
			for(l := lids; l != nil; l = tl l)
				kill((hd l).t2);
			kill(pidr);
			exit;
		}
	}
}

history(ctxt: ref Draw->Context, id: string, ci: chan of int, sync: chan of int, exitc: chan of int)
{
	sync <-= sys->pctl(0, nil);
	(win, wmch) := tkclient->toplevel(ctxt, "", "Db"+id, Tkclient->Resize | Tkclient->Hide);
	cmdch := chan of string;
	tk->namechan(win, cmdch, "cmd");
	for(i := 0; i < len winconfigh; i++)
		cmd(win, winconfigh[i]);
	tkclient->onscreen(win, nil);
	tkclient->startinput(win, "kbd"::"ptr"::nil);
	disp := win.image.display;
	(img, maxx, maxy) := newimage(win, ".p");
	black := disp.rgb(0,0,0);
	yellow := disp.color(Draw->Yellow);

	db := array[NDBS] of int;
	for(i = 0; i < NDBS; i++)
		db[i] = rand->rand(5)+9;
	n := NDBS-1;
	dx := real maxx/real NDBS;
	dy := real maxy/real (MAXDB-MINDB);
	update := 1;
	mindb := MAXDB;
	maxdb := MINDB;

	for(;;){
		alt{
		c := <-win.ctxt.kbd =>
			tk->keyboard(win, c);
		c := <-win.ctxt.ptr =>
			tk->pointer(win, *c);
		c := <-win.ctxt.ctl or
		c = <-win.wreq =>
			tkclient->wmctl(win, c);
		c := <- wmch =>
			case(c){
			"exit" =>
				exitc <-= 0;
				exit;
			* =>
				tkclient->wmctl(win, c);
				if(c[0] == '!'){
					(img, maxx, maxy) = newimage(win, ".p");
					dx = real maxx/real NDBS;
					dy = real maxy/real (MAXDB-MINDB);
					update = 1;
				}
			}
		<- cmdch =>
			;
		v := <-ci =>
			if(update){	# bug if not done here
				hdraw(img, yellow, black, db, n, dx, dy, maxy);
				update = 0;
			}
			if(v < mindb)
				mindb = v;
			if(v > maxdb)
				maxdb = v;
			v -= MINDB;
			db[n] = v;
			if(++n == NDBS)
				n = 0;
			x := int dx;
			y := int(real v*dy);
			img.draw(img.r, img, nil, (x, 0));
			img.draw(Rect((maxx-x, 0), (maxx, maxy-y)), black, nil, (0, 0));
			img.draw(Rect((maxx-x, maxy-y), (maxx, maxy)), yellow, nil, (0, 0));
			reveal(win, ".p");
		}
	}
}

reader(fd: ref Sys->FD, cp: chan of (int, int, int), sync: chan of int)
{
	sync <-= sys->pctl(0, nil);
	syncs := 0;
	b := array[2] of { byte 16rfe, byte 16rdc };
	buf := array[4+NDB] of byte;
	for(;;){
		if(syncs < 2){
			c := get1(fd, buf);
			if(c == b[syncs])
				syncs++;
			else
				syncs = 0;
			continue;
		}
		getn(fd, buf, 4+NDB);
		id := get2(buf, 0);
		t := get2(buf, 2);
		for(i := 0; i < NDB; i++){
			db := (int buf[i+4])&16rff;
			cp <-= (id, t, db);
		}
		syncs = 0;
	}
}

hdraw(img: ref Image, fg: ref Image, bg: ref Image, a: array of int, n: int, dx: real, dy: real, maxy: int)
{
	img.draw(img.r, bg, nil, Point(0, 0));
	m := len a;
	x0 := 0.0;
	for(i := 0; i < m; i++){
		x1 := x0+dx;
		y := int(real a[n]*dy);
		if(++n == m)
			n = 0;
		img.draw(Rect((int x0, maxy-y), (int x1, maxy)), fg, nil, (0, 0));
		x0 = x1;
	}
}

T(db: int, maxy: int): int
{
	return int(real(db-MINDB)*real maxy/real(MAXDB-MINDB));
}

get1(fd: ref Sys->FD, b: array of byte): byte
{
	n := sys->read(fd, b, 1);
	if(n != 1)
		fatal("read error");
	return b[0];
}

get2(b: array of byte, k: int): int
{
	return int b[k]<<8 | int b[k+1]<<0;
}

get4(b: array of byte, k: int): int
{
	return int b[k]<<24 | int b[k+1]<<16 | int b[k+2]<<8 | int b[k+3]<<0;
}

gets1(b: array of byte, k: int): int
{
	v := int b[k];
	if(v&(1<<7))
		v |= int 16rffffff00;
	return v;
}

gets2(b: array of byte, k: int): int
{
	v := get2(b, k);
	if(v&(1<<15))
		v |= int 16rffff0000;
	return v;
}

getn(fd: ref Sys->FD, b: array of byte, n: int)
{
	while(n > 0){
		m := sys->read(fd, b, n);
		if(m <= 0)
			fatal("read error");
		b = b[m: ];
		n -= m;
	}
}

cmd(top: ref Toplevel, s: string): string
{
	e := tk->cmd(top, s);
	if (e != nil && e[0] == '!')
		fatal(sys->sprint("tk error on '%s': %s", s, e));
	return e;
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

fatal(e: string)
{
	sys->fprint(sys->fildes(2), "%s\n", e);
	exit;
}

winconfigh := array[] of {
	"panel .p -width " + string MAXXH + " -height " + string MAXYH + " -bd 2 -relief flat",
	"pack .p -side left -fill both -expand 1",
	"pack propagate . 0",
	"update"
};
