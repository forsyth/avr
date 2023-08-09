ResampleMod: module {
	PATH: con "/dis/mote/resamplemod.dis";

	init: fn (ctxt: ref Draw->Context, vx, vy, px, py: int, img: ref Image, imgpath: string): (ref Image, string);
};
