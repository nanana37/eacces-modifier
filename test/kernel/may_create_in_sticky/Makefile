all:
	gcc main.c && ./a.out
	gcc other.c -o other
	$(info *** Do "./other" as other user ***)

other:
	gcc other.c -o other


clean:
	rm -f a.out other
	sudo rm -rf out/*


