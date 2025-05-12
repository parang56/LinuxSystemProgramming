CC = gcc

HEADER = ssu_header
HELP = ssu_help
ADD = ssu_add
STATUS = ssu_status
COMMIT = ssu_commit
BACKUP = ssu_repo
STRUCT = ssu_struct
INIT = ssu_init
LOG = ssu_log
REMOVE = ssu_remove

$(BACKUP) : $(BACKUP).o 
	$(CC) -o $(BACKUP) $(BACKUP).o -lcrypto

$(BACKUP).o : $(HEADER).h $(HELP).h $(LOG).h $(STATUS).h $(COMMIT).h $(ADD).h $(STRUCT).h $(INIT).h $(REMOVE).h $(BACKUP).c
	$(CC) -c -o $@ $(BACKUP).c -lcrypto
	
	
clean :
	rm -rf $(BACKUP)
	rm -rf *.o
