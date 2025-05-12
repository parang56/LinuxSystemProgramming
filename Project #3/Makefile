HEADER = ssu_header
HELP = ssu_help
ADD = ssu_add
SYNC = ssu_sync
STRUCT = ssu_struct
LIST = ssu_list
REMOVE = ssu_remove

$(SYNC) : $(SYNC).o $(HEADER).o $(HELP).o $(ADD).o $(STRUCT).o $(LIST).o $(REMOVE).o
	$(CC) -g -o $@ $^ -lcrypto -fcommon

$(SYNC).o : $(SYNC).c
	$(CC) -c -o $@ $^ -lcrypto -fcommon
	
$(HEADER).o : $(HEADER).c
	$(CC) -c -o $@ $^ -lcrypto -fcommon
	
$(HELP).o : $(HELP).c
	$(CC) -c -o $@ $^ -lcrypto -fcommon
	
$(ADD).o : $(ADD).c
	$(CC) -c -o $@ $^ -lcrypto -fcommon

$(STRUCT).o : $(STRUCT).c
	$(CC) -c -o $@ $^ -lcrypto -fcommon
	
$(REMOVE).o : $(REMOVE).c
	$(CC) -c -o $@ $^ -lcrypto -fcommon

clean :
	rm -rf $(SYNC)
	rm -rf *.o
