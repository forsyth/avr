</$objtype/mkfile

DIRS=cc za zc zi zl os libc

dirs:V:
	for(i in cc $DIRS) @{
		cd $i
		echo mk $i
		mk $MKFLAGS all
	}

install:V:
	for(i in $DIRS) @{
		cd $i
		echo mk $i
		mk $MKFLAGS install
	}

clean:V:
	for(i in $DIRS) @{
		cd $i
		echo mk $i clean
		mk $MKFLAGS clean
	}
