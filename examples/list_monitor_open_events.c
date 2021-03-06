#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/select.h>

#include <libspacemouse.h>

int main()
{
  struct spacemouse *iter, *mon_mouse;
  spacemouse_event_t mouse_event;
  fd_set fds;
  int mouse_fd, monitor_fd, max_fd;
  int action, read_event;

  monitor_fd = spacemouse_monitor_open();

  spacemouse_device_list(&iter, 1);
  if (iter == NULL)
    printf("No devices found.\n");

  spacemouse_device_list_foreach(iter, iter) {
    printf("device id: %d\n", spacemouse_device_get_id(iter));
    printf("  devnode: %s\n", spacemouse_device_get_devnode(iter));
    printf("  manufacturer: %s\n", spacemouse_device_get_manufacturer(iter));
    printf("  product: %s\n", spacemouse_device_get_product(iter));

    if (spacemouse_device_open(iter) < 0)
      fprintf(stderr, "Failed to open device: %s\n",
              spacemouse_device_get_devnode(iter));

    spacemouse_device_set_led(iter, 1);
  }

  while(1) {
    FD_ZERO(&fds);
    FD_SET(monitor_fd, &fds);
    max_fd = monitor_fd;

    spacemouse_device_list(&iter, 0);
    spacemouse_device_list_foreach(iter, iter)
      if ((mouse_fd = spacemouse_device_get_fd(iter)) > -1) {
        FD_SET(mouse_fd, &fds);
        if (mouse_fd > max_fd) max_fd = mouse_fd;
      }

    if (select(max_fd + 1, &fds, NULL, NULL, NULL) == -1) {
      perror("select");
      break;
    }

    if (FD_ISSET(monitor_fd, &fds)) {
      action = spacemouse_monitor(&mon_mouse);

      if (action == SPACEMOUSE_ACTION_ADD) {
        printf("Device added, ");

        spacemouse_device_open(mon_mouse);

        spacemouse_device_set_led(mon_mouse, 1);
      } else if (action == SPACEMOUSE_ACTION_REMOVE) {
        printf("Device removed, ");

        spacemouse_device_close(mon_mouse);
      }

      if (action == SPACEMOUSE_ACTION_ADD ||
          action == SPACEMOUSE_ACTION_REMOVE) {
        printf("device id: %d\n", spacemouse_device_get_id(mon_mouse));
        printf("  devnode: %s\n", spacemouse_device_get_devnode(mon_mouse));
        printf("  manufacturer: %s\n",
               spacemouse_device_get_manufacturer(mon_mouse));
        printf("  product: %s\n", spacemouse_device_get_product(mon_mouse));
      }
    }

    spacemouse_device_list(&iter, 0);
    spacemouse_device_list_foreach(iter, iter) {
      mouse_fd = spacemouse_device_get_fd(iter);
      if (mouse_fd > -1 && FD_ISSET(mouse_fd, &fds)) {
        memset(&mouse_event, 0, sizeof mouse_event);

        read_event = spacemouse_device_read_event(iter, &mouse_event);
        if (read_event == -1)
          spacemouse_device_close(iter);
        else if (read_event == SPACEMOUSE_READ_SUCCESS) {
          printf("device id %d: ", spacemouse_device_get_id(iter));

          if (mouse_event.type == SPACEMOUSE_EVENT_MOTION) {
            printf("got motion event: t(%d, %d, %d) ",
                   mouse_event.motion.x, mouse_event.motion.y,
                   mouse_event.motion.z);
            printf("r(%d, %d, %d) period(%d)\n",
                   mouse_event.motion.rx, mouse_event.motion.ry,
                   mouse_event.motion.rz, mouse_event.motion.period);
          } else if (mouse_event.type == SPACEMOUSE_EVENT_BUTTON) {
            printf("got button %s event b(%d)\n",
                   mouse_event.button.press ? "press" : "release",
                   mouse_event.button.bnum);
          }
        }
      }
    }
  }

  return 0;
}
