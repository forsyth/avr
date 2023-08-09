implement MoteRead;

include "sys.m";
	sys: Sys;
include "draw.m";
	draw: Draw;
	Display, Image, Font, Rect, Point, Endsquare: import draw;
include "arg.m";
include "daytime.m";
	daytime: Daytime;
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

NVALS: con 2;
TLEN: con 3;
BPR: con TLEN + NVALS;
NULLDBVALUE: con 255;
resultsdir: string;

MoteRead: module
{
	init: fn(nil: ref Draw->Context, nil: list of string);
};

init(nil: ref Draw->Context, args: list of string)
{
	sys = load Sys Sys->PATH;
	draw = load Draw Draw->PATH;
	tk = load Tk Tk->PATH;
	tkclient = load Tkclient Tkclient->PATH;
	daytime = load Daytime Daytime->PATH;
	arg := load Arg Arg->PATH;

	if(sys->bind("#t", "/dev", Sys->MAFTER) < 0)
		;	# fatal("cannot bind serial device");
	arg->init(args);
	arg->setusage("serial [-d serialdev] resultsdir");
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
	args = arg->argv();
	if (len args != 1)
		arg->usage();	
	resultsdir = hd args;
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
	cr := chan of (int, int, array of int);
	syncr := chan of int;
	spawn reader(sfd, cr, syncr);
	pidr = <-syncr;

	lids: list of (int, chan of (int, array of int), int);

	for(;;){
		alt{
		(id, t, dbs) := <-cr =>
			c: chan of (int, array of int);

			for(l := lids; l != nil; l = tl l)
				if((hd l).t0 == id)
					break;
			if(l == nil){
				pid := -1;
				c = chan of (int, array of int);
				sync := chan of int;
				spawn writer(id, daytime->now() - t, c, sync);
				pid = <-sync;
				lids = (id, c, pid) :: lids;
			}
			else
				c = (hd l).t1;
			if(c != nil)
				c <-= (t, dbs);
		}
	}
}

writer(id, starttime: int, ci: chan of (int, array of int), sync: chan of int)
{
	sync <-= sys->pctl(0, nil);
	fd := sys->create(resultsdir + "/" + string id, sys->OWRITE, 8r666);
	if (fd == nil) {
		sys->fprint(sys->fildes(2), "Failed to create results file: %s/%d: %r\n", resultsdir, id);
		return;
	}
	buf := array[4] of byte;
	int2byte4(starttime, buf);
	sys->write(fd, buf, 4);
	
	buf = array[BPR] of byte;
	isdead := 0;

	for(;;){
		alt{
		(t, v) := <-ci =>
			if (isdead)
				break;
			int2byte3(t, buf);
			s := string id + ": " + string t + "= (";
			for (i := 0; i < len v; i++) {
				if(v[i] <= 0 || isdead) {
					v[i] = NULLDBVALUE;
					isdead = 1;
				}
				buf[TLEN + i] = byte v[i];
				s += string v[i] + ",";
			}
			s = s[:len s - 1] + ")";
			sys->print("%s\n", s);
			sys->write(fd, buf, BPR);
		}
	}
}

reader(fd: ref Sys->FD, cp: chan of (int, int, array of int), sync: chan of int)
{
	sync <-= sys->pctl(0, nil);
	syncs := 0;
	b := array[2] of { byte 16rfe, byte 16rdc };
	buf := array[6] of byte;
	dbs := array[NVALS] of int;
	for(;;){
		if(syncs < 2){
			c := get1(fd, buf);
			if(c == b[syncs])
				syncs++;
			else
				syncs = 0;
			continue;
		}
		getn(fd, buf, 4 + NVALS);
		id := get2(buf, 0);
		t := get2(buf, 2);
		# db := get2(buf, 4);
		dbs = array[NVALS] of int;
		for (i := 0; i < NVALS; i++)
			dbs[i] = (int buf[4+i])&16rff;
		cp <-= (id, t, dbs);
		syncs = 0;
	}
}

T(db: int, maxy: int): int
{
	return int(real(db-MINDB)*real maxy/real(MAXDB-MINDB));
}

int2byte4(i: int, buf: array of byte)
{
	buf[0] = byte i;
	buf[1] = byte (i>>8);
	buf[2] = byte (i>>16);
	buf[3] = byte (i>>24);
}

int2byte3(i: int, buf: array of byte)
{
	buf[0] = byte i;
	buf[1] = byte (i>>8);
	buf[2] = byte (i>>16);
}

get1(fd: ref Sys->FD, b: array of byte): byte
{
	sys->read(fd, b, 1);
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
