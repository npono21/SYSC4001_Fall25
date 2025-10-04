if [ ! -d "bin" ]; then
    mkdir bin
else
	rm bin/*
fi
gcc interrupts.c -o bin/interrupts.exe
end