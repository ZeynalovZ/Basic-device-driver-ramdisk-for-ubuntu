#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h> /* printk() */
#include <linux/fs.h>     /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>


static int sectorsize=512;
static int nsectors=2;
static int major_num=0;


static struct request_queue *queue;

static struct blockdevice {
	unsigned long size;
    u8 *data;
    short users;
    spinlock_t lock;
    struct gendisk *gd;
}device;

static void sbd_transfer(struct blockdevice *dev, sector_t sector,
    unsigned long nsect, char *buffer, int write) {
   unsigned long offset=sector*512;
  unsigned long nbytes = nsect * 512;
   
  if ((offset + nbytes) > dev->size) {
    printk (KERN_NOTICE "sbd: Beyond-end write (%ld %ld)\n", offset, nbytes);
    return;
  }
  if (write) 
    memcpy(dev->data + offset, buffer, nbytes);

  else
    memcpy(buffer, dev->data + offset, nbytes);

}



static void handle_request(struct request_queue *q) {
  struct request *req;
  req = blk_fetch_request(q);
  while (req != NULL) {

    if (req == NULL || (req->cmd_type != REQ_TYPE_FS)) {
      printk (KERN_NOTICE "Skip non-CMD request\n");
      __blk_end_request_all(req, -EIO);
      continue;
    }
    sbd_transfer(&device, blk_rq_pos(req), blk_rq_cur_sectors(req),
bio_data(req->bio), rq_data_dir(req));
    if ( ! __blk_end_request_cur(req, 0) ) {
      req = blk_fetch_request(q);
    }
  }
}


 static int blkdev_open(struct block_device * inode, fmode_t t)  {
 	
 	  unsigned unit = iminor(inode->bd_inode);
     spin_lock(&device.lock); 
     device.users++;  
     
     printk(KERN_NOTICE "device was opened");
          printk(KERN_INFO "rb: Inode number is %d\n", unit);
      printk(KERN_NOTICE "No.of current users = %d",device.users);
       spin_unlock(&device.lock); 
     return 0;
 }


 static int blkdev_close(struct gendisk *disk, fmode_t mode)
{ 
	 spin_lock(&device.lock); 
	device.users--;
    printk(KERN_INFO "rb: Device is closed\n");
    printk(KERN_NOTICE "No.of current users = %d",device.users);
     spin_unlock(&device.lock); 
    return 0;
}

static struct block_device_operations blockoperations=  {
	    
		.owner  = THIS_MODULE,
		.open =blkdev_open,
		.release=blkdev_close,
};



static int __init block_init(void) {
	/* Setting up the device memory and structure*/
	device.size=nsectors*sectorsize;
    spin_lock_init(&device.lock);
    device.data=vmalloc(device.size);
    if(device.data==NULL) {
        printk(KERN_WARNING "Valloc has failed");
        return -ENOMEM;
    }
    

    /*setting up the request queue for the device */
    queue=blk_init_queue(handle_request,&device.lock);
    if(queue==NULL) {
         printk(KERN_WARNING "Request queue allocation failed");
         goto out;
    }


    /*getting a major number for the device */
    major_num=register_blkdev(0,"magicbox");
    if(major_num<=0) {
       printk(KERN_WARNING"Unable to get major number");
       goto out;
    }

    /* Setting up the gendisk structure */
    device.gd=alloc_disk(1);
    if(!device.gd) {
       printk(KERN_WARNING "Unable to get gendisk structure");
       goto out_unregister;
    }
    device.gd->major=major_num;
    device.gd->first_minor=0;
    device.gd->fops=&blockoperations;
    device.gd->private_data=&device;
    set_capacity(device.gd, nsectors);
    strcpy(device.gd->disk_name,"magicbox");
    device.gd->queue=queue;

   

    add_disk(device.gd);
        printk(KERN_WARNING "Done succesfully\n");
	return 0;

   out_unregister:
       unregister_blkdev(major_num,"magicbox");
   out:
       vfree(device.data);
       return -ENOMEM;
}

static void __exit block_exit(void) {
  del_gendisk(device.gd);
	put_disk(device.gd);
	unregister_blkdev(major_num, "magicbox");
	blk_cleanup_queue(queue);
	vfree(device.data);	
	printk(KERN_WARNING "Closing the module");

}

module_init(block_init);
module_exit(block_exit);  



MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Sudhakar Reddy <email@knssr23@gmail.com>");
MODULE_DESCRIPTION("Block device driver (implemented on ram )");
