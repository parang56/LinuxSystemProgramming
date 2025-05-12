CC = gcc

HEADER = ssu_header
HEADER_BACKUP = backup
HEADER_REMOVE = remove
HEADER_RECOVER = recover
HEADER_HELP = ssu_help
BACKUP = ssu_backup

$(BACKUP) : $(BACKUP).o
	$(CC) -o $(BACKUP) $(BACKUP).o -lcrypto

$(BACKUP).o : $(HEADER).h $(HEADER_BACKUP).h $(HEADER_REMOVE).h $(HEADER_RECOVER).h $(HEADER_HELP).h $(BACKUP).c
	$(CC) -c -o $@ $(BACKUP).c -lcrypto
	
clean :
	rm -rf $(BACKUP)
	rm -rf *.o
