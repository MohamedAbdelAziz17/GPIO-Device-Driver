#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/gpio.h>

/*Meta Information*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MohamedGalal");
MODULE_DESCRIPTION("A hello world Psuedo device driver");

/*Prototypes*/
static int driver_open (struct inode *device_file, struct file *instance);
static int driver_close (struct inode *device_file, struct file *instance);
ssize_t driver_read (struct file * file, char __user *user_buffer , size_t count, loff_t *off);
ssize_t driver_write (struct file * file, const char __user * user_buffer, size_t count, loff_t * off);


/*Declarations*/
#define LED_PIN 2

/********GPIO********/
//=gpio pin
unsigned int pin_number;
module_param(pin_number,uint,0660);
//='o' outupt , 'i' input
char* configuration;
module_param(configuration,charp,0660);

struct class* myClass;
struct cdev st_character_device;
struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open  = driver_open,
    .release = driver_close,
    .read  = driver_read,
    .write = driver_write
};
dev_t device_number;
int major_number = 0;
struct device* mydevice;
#define BUFF_SIZE 15
#define READ_SIZE 3
static unsigned char buffer[BUFF_SIZE] = ""; // this at kernalspace

/*Implementations*/                                 // this at userspace
ssize_t driver_read (struct file * file, char __user *user_buffer , size_t count, loff_t *off)
{
    int not_copied;
    char tmp[3] = "";

    printk("%s: the count to read %d \n",__func__, count);
    printk("%s: the offset %lld \n",__func__, *off);
    if(count + *off > READ_SIZE){
        count = READ_SIZE - *off;
    }

    tmp[0] = gpio_get_value(pin_number) + 48; //convert to ascii
    tmp[1] = '\n';
    
    printk("pin level: %c\n", tmp[0]);
    not_copied  = copy_to_user(user_buffer,&tmp[*off],count);

    if(not_copied){
        return -1;
    }
    
    *off = count;
    printk("%s: not copied %d \n",__func__, not_copied);
    printk("%s: message --> %s \n", __func__ , user_buffer);
    return count;
}

ssize_t driver_write (struct file * file, const char __user * user_buffer, size_t count, loff_t * off)
{
    int not_copied;

    printk("%s: the count to write %d \n",__func__, count);
    printk("%s: the offset %lld \n",__func__, *off);
    if(count + *off > BUFF_SIZE){
        count = BUFF_SIZE - *off;
    }
    if(count == 0){
        printk("no space left\n");
        return -1;
    }
    not_copied  = copy_from_user(&buffer[*off], user_buffer, count);
    if(configuration[0] != 'o'){
        printk("cannot write to input configured pin!\n");
        return -1;
    }
    switch(user_buffer[0]){
        case '1':
        gpio_set_value(pin_number,1);
        break;
        case '0':
        gpio_set_value(pin_number,0);
        break;
    }
    if(not_copied){
        return -1;
    }
    *off = count;
    printk("%s: already done %d \n",__func__, count);
    printk("%s: message --> %s \n", __func__ , buffer);
    return count;
}

/**
 * @brief This function is called, when the module is loaded into the kernel
 */
static int __init hellodrive_init(void)
{
    //define local variables
    int retval;

    //1- allocate device number
    retval = alloc_chrdev_region(&device_number,0,1,"test_devicenumber");
    if(retval == 0)
    {
        printk("%s retval=0 registered! - major: %d Minor: %d\n",__FUNCTION__,MAJOR(device_number),MINOR(device_number));
    }
    else{
        printk("could not register device number\n");
        return -1;
    }

    //2- define driver character or block or network
    cdev_init(&st_character_device, &fops);
    retval = cdev_add(&st_character_device , device_number , 1);
    if(retval != 0){
        printk("Registering device is failed!\n");
        goto CHARACTER_ERROR;
    }

    //3- generate file ( class - device )
    myClass = class_create(THIS_MODULE,"test_class");
    if(myClass == NULL){
        printk("Device class failed!\n");
        goto CLASS_ERROR;
    }
    //4- create the device file
    mydevice = device_create(myClass,NULL,device_number,NULL,"myGPIO");
    
    if(mydevice == NULL){
        printk("Device create failed!\n");
        goto DEVICE_ERROR;
    }
    //GPIO request
    if(pin_number < 0 || pin_number > 27){
        printk("Wrong GPIO pin number input\n");
        goto GPIO_REQ_ERROR;
    }
    gpio_request(pin_number,"rpi_gpio");
    //GPIO direction
    if(configuration[0] == 'o'){
        gpio_direction_output(pin_number,0);
    }else if(configuration[0] == 'i'){
        gpio_direction_input(pin_number);
    }else{
        printk("Wrong GPIO pin configuration\n");
        goto GPIO_DIR_ERROR;
    }
    
    // if(gpio_request(LED_PIN,"rpi_gpio_2")){
    //     printk("gpio request failed\n");
    //     goto GPIO_REQ_ERROR;
    // }
    // if(gpio_direction_output(LED_PIN,0)){
    //     printk("gpio direction failed\n");
    //     goto GPIO_DIR_ERROR;
    // }
    return 0;

    //ERROR Hanlding
    GPIO_DIR_ERROR:
        gpio_free(pin_number);
    GPIO_REQ_ERROR:
        device_destroy(myClass,device_number);
    DEVICE_ERROR:
        cdev_del(&st_character_device);
    CLASS_ERROR:
        class_destroy(myClass);
    CHARACTER_ERROR:
        unregister_chrdev_region(device_number,1);
        return -1;
}
/**
 * @brief This function is called, when the module is removed from the kernel
 */
static void __exit hellodrive_exit(void)
{
    //unregister_chrdev(major_number,"mytest_driver");
    gpio_set_value(pin_number,0);
    gpio_free(pin_number);
    cdev_del(&st_character_device);  //delete device number
    device_destroy(myClass,device_number); //delete device
    class_destroy(myClass); //delete class
    unregister_chrdev_region(device_number,1); //unregister
    printk("Module exit!\n");
}

static int driver_open(struct inode *device_file, struct file *instance)
{
    printk("%s dev_nr - open was called!\n",__FUNCTION__);
    return 0;
}
static int driver_close(struct inode *device_file, struct file *instance)
{
    printk("%s dev_nr - close was called!\n",__FUNCTION__);
    return 0;
}

// Acts as constructor of the module
module_init(hellodrive_init);
// Acts as deconstructor of the module
module_exit(hellodrive_exit);