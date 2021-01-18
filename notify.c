#include<string.h>
#include<libnotify/notify.h>

void send_notification(unsigned char * topic, unsigned char * message) {
  if (topic == NULL || message == NULL) {
	return;
  }
  notify_init("chat notification");

  NotifyNotification *notif = notify_notification_new(topic, message, NULL);
  
  GError *error = NULL;
  notify_notification_show(notif, &error);
}

