/* datetime.c */
void getdatestring_now(char *datestring);

/* post.c */
int postfile(char *filename, char *board, char *title, char *owner, int flag);
int postmail(char *filename, char *user, char *title, char *owner, int flag, int check_permission);

/* record.c */
int safewrite(int fd, void *buf, int size);
int get_num_records(char *filename, int size);
int append_record(char *filename, void *record, int size);
int search_record_forward(char *filename, void *rptr, int size, int start, int (*fptr)(void *, void *), void *farg);
int delete_record(char *filename, int size, int id);
int apply_record(char *filename, int (*fptr)(void *, int), int size);
int get_record(char *filename, void *rptr, int size, int id);
int get_records(char *filename, void *rptr, int size, int id, int number);
int substitute_record(char *filename, void *rptr, int size, int id);
int safe_substitute_record(char *direct, struct fileheader *fhdr, int size, int ent);
