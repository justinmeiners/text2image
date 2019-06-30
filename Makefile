text2image: text2image.c stb_image_write.h stb_truetype.h cmunrm.h
	gcc -Os text2image.c -o $@

cmunrm.h: 
	xxd -i font/cmunrm.ttf > cmunrm.h

sample.png: text2image
	echo "bob@email.com" | ./text2image > sample.png

