run: ht_main.c ht.c
	gcc -o run ht_main.c ht.c BF_64.a -static
clean:
	rm -f run
