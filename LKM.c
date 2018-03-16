/*
Programming Assignment 2
COP 4600 Spring 18
Group 50
Christopher Hodge
Michael Jones
Marlee Knotts
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#define  DEVICE_NAME "groupFiftyLDriver"
#define  CLASS_NAME  "groupFifty"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group 50");
MODULE_DESCRIPTION("Character-mode Linux device driver as kernel module for PA 2");
MODULE_VERSION("1.0");

static int    majorNumber;
static char   message[256] = {0};
static short  size_of_message;
static int    numberOpens = 0;
static struct class*  groupFiftycharClass  = NULL;
static struct device* groupFiftycharDevice = NULL;

// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};

static int __init ebbchar_init(void){
   printk(KERN_INFO "groupFiftyLDriver: Initializing the groupFiftyLDriver LKM\n");

   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "groupFiftyLDriver failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "groupFiftyLDriver: registered correctly with major number %d\n", majorNumber);

   // Register the device class
   groupFiftycharClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(groupFiftycharClass)){                // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(groupFiftycharClass);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "groupFiftyLDriver: device class registered correctly\n");

   // Register the device driver
   groupFiftycharDevice = device_create(groupFiftycharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(groupFiftycharDevice)){               // Clean up if there is an error
      class_destroy(groupFiftycharClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(groupFiftycharDevice);
   }
   printk(KERN_INFO "groupFiftyLDriver: device class created correctly\n"); // Made it! device was initialized
   return 0;
}

static void __exit ebbchar_exit(void){
   device_destroy(groupFiftycharClass, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(groupFiftycharClass);                          // unregister the device class
   class_destroy(groupFiftycharClass);                             // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
   printk(KERN_INFO "groupFiftyLDriver: Goodbye from the LKM!\n");
}

static int dev_open(struct inode *inodep, struct file *filep){
   numberOpens++;
   printk(KERN_INFO "groupFiftyLDriver: Device has been opened %d time(s)\n", numberOpens);
   return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   int error_count = 0;
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   error_count = copy_to_user(buffer, message, size_of_message);

   if (error_count==0){            // if true then have success
      printk(KERN_INFO "groupFiftyLDriver: Sent %d characters to the user\n", size_of_message);
      return (size_of_message=0);  // clear the position to the start and return 0
   }
   else {
      printk(KERN_INFO "groupFiftyLDriver: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
   }
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
   sprintf(message, "%s(%zu letters)", buffer, len);   // appending received string with its length
   size_of_message = strlen(message);                 // store the length of the stored message
   printk(KERN_INFO "groupFiftyLDriver: Received %zu characters from the user\n", len);
   return len;
}

static int dev_release(struct inode *inodep, struct file *filep){
   printk(KERN_INFO "groupFiftyLDriver: Device successfully closed\n");
   return 0;
}

module_init(ebbchar_init);
module_exit(ebbchar_exit);
