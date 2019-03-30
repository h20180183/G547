#include<linux/kernel.h>
#include<linux/blkdev.h>
#include<linux/bio.h>
#include<linux/genhd.h>
#include<linux/types.h>
#include<linux/init.h>
#include<linux/module.h>
#include<linux/fs.h>
#include<linux/errno.h>
#include<linux/uaccess.h>
#include<linux/sched.h>
#include<linux/workqueue.h>

#define FILE_PATH "/etc/sample.txt"

#define FIRST_MINOR 1
#define DEV_NAME "dof"
#define MINORS 2
#define DEV_NR_SEC 1024
#define DEV_SEC_SIZE 512
#define KERNEL_SEC_SIZE 512

#define SECTOR_SIZE 512
#define MBR_SIZE SECTOR_SIZE
#define MBR_DISK_SIGNATURE_OFFSET 440
#define MBR_DISK_SIGNATURE_SIZE 4
#define PARTITION_TABLE_OFFSET 446
#define PARTITION_ENTRY_SIZE 16 // sizeof(PartEntry)
#define PARTITION_TABLE_SIZE 64 // sizeof(PartTable)
#define MBR_SIGNATURE_OFFSET 510
#define MBR_SIGNATURE_SIZE 2
#define MBR_SIGNATURE 0xAA55
#define BR_SIZE SECTOR_SIZE
#define BR_SIGNATURE_OFFSET 510
#define BR_SIGNATURE_SIZE 2
#define BR_SIGNATURE 0xAA55


typedef struct //structure for partition table
{
    unsigned char boot_type; // 0x00 - Inactive; 0x80 - Active (Bootable)
    unsigned char start_head;
    unsigned char start_sec:6;
    unsigned char start_cyl_hi:2;
    unsigned char start_cyl;
    unsigned char part_type;
    unsigned char end_head;
    unsigned char end_sec:6;
    unsigned char end_cyl_hi:2;
    unsigned char end_cyl;
    unsigned int abs_start_sec;
    unsigned int sec_in_part;
} PartEntry;

typedef PartEntry PartTable[4];

static PartTable def_part_table =
{
    {
        boot_type: 0x00,
        start_head: 0x00,
        start_sec: 0x2,
        start_cyl: 0x00,
        part_type: 0x83,
        end_head: 0x00,
        end_sec: 0x20,
        end_cyl: 0x00,
        abs_start_sec: 0x00000001,
        sec_in_part: 0x00000200
    },
    {
        boot_type: 0x00,
        start_head: 0x00,
        start_sec: 0x1,
        start_cyl: 0x14,
        part_type: 0x83,
        end_head: 0x00,
        end_sec: 0x20,
        end_cyl: 0x1F,
        abs_start_sec: 0x00000280,
        sec_in_part: 0x00000200
    },
    {
      boot_type: 0x00,
      start_head: 0x00,
      start_sec: 0x0,
      start_cyl: 0x00,
      part_type: 0x00,
      end_head: 0x00,
      end_sec: 0x00,
      end_cyl: 0x00,
      abs_start_sec: 0x00000000,
      sec_in_part: 0x00000000
    },
    {
      boot_type: 0x00,
      start_head: 0x00,
      start_sec: 0x0,
      start_cyl: 0x00,
      part_type: 0x00,
      end_head: 0x00,
      end_sec: 0x00,
      end_cyl: 0x00,
      abs_start_sec: 0x00000000,
      sec_in_part: 0x00000000
    }
};
/*------------------------------------------------------------------*/
int major;

struct block_device_operations my_block_ops ={
  .owner = THIS_MODULE
};
struct my_block_struct
{
  spinlock_t lock;
  struct gendisk* gd;
  struct request_queue* queue;
};

struct my_block_struct* device = NULL;

struct my_work_struct
{
  struct work_struct work;
  sector_t sector;
  unsigned char* buffer;
  size_t len;
}work_read,work_write;


/*------------------------------------------------------------------*/

int copy_mbr(struct file* f)
{
  unsigned char* dummy;
  unsigned char signature[2] = {0x55,0xAA};
  loff_t zero =0;
  unsigned int i=0;
  unsigned char buf[2024];
  ssize_t res =0;
  dummy = kzalloc(PARTITION_TABLE_OFFSET,GFP_KERNEL);
  if((res = kernel_write(f,dummy,PARTITION_TABLE_OFFSET,&zero))<0)
  {
    printk(KERN_INFO"MBR creation failed - Bootstrap\n");
    return -ENOMEM;
  }
  printk(KERN_INFO"MBR - %d\n",(int)res);
  zero = PARTITION_TABLE_OFFSET;
  if((res = kernel_write(f,&def_part_table,sizeof(def_part_table),&zero))<0)
  {
    printk(KERN_INFO"MBR creation failed - Partition table\n");
    return -ENOMEM;
  }
  zero = MBR_SIGNATURE_OFFSET;
  printk(KERN_INFO"MBR - %d\n",res);
  if((res = kernel_write(f,signature,2,&zero))<0)
  {
    printk(KERN_INFO"MBR creation failed - Signature\n");
    return -ENOMEM;
  }
  printk(KERN_INFO"MBR - %d\n",(int)res);
  zero =0;
  printk("MBR write is successful\n");
  res = kernel_read(f,buf,512,&zero);
  for(i=0;i<=511;i++)
  {printk("%x\n",*(buf+i));}
  return 0;
}
/*------------------------------------------------------------------*/

static void block_write(struct work_struct *work)
{
  struct my_work_struct * mwp = container_of(work,struct my_work_struct,work);
  struct file* f = filp_open(FILE_PATH,O_RDWR,0666);
  size_t res;
/*  if(f->f_pos <= 512)
  {
    f->f_pos = (loff_t)4096;
  }*/
  printk(KERN_INFO"Inside - Write deferred : %d bytes from %d\n",(int)mwp->len,(int)f->f_pos);
  if((res = kernel_write(f,mwp->buffer,mwp->len,&f->f_pos))<0)
  {
    printk(KERN_INFO"Write request failed\n");
    res = -ENOMEM;
  }
  printk(KERN_INFO"Write complete : %d bytes\n",res);
  filp_close(f,NULL);
  return;
}


static void block_read(struct work_struct *work)
{
  struct my_work_struct * mwp = container_of(work,struct my_work_struct,work);
  struct file* f = filp_open(FILE_PATH,O_RDWR,0666);
  size_t res;
  loff_t pos = (mwp->sector)*DEV_SEC_SIZE;
  printk(KERN_INFO"Inside - Read deferred : %d bytes\n",(int)mwp->len);
  if((res = kernel_read(f,mwp->buffer,mwp->len,&pos))<0)
  {
    printk(KERN_INFO"Read request failed\n");
    res = -ENOMEM;
  }
  printk(KERN_INFO"Read complete of sector %d : %d bytes\n",(int)mwp->sector,(int)res);
  filp_close(f,NULL);
  return;
}

/*------------------------------------------------------------------*/

void block_request(struct request_queue* que)
{
  struct request* rq;
  struct req_iterator iter;
  struct bio_vec bvec;
  rq = blk_fetch_request(que);
  while(rq != NULL)
  {
    if(blk_rq_is_passthrough(rq))
    {
			printk(KERN_INFO "non FS request\n");
			__blk_end_request_all(rq, -EIO);
			continue;
		}
    else
    {
      rq_for_each_segment(bvec,rq,iter)
      {
        sector_t sector = iter.iter.bi_sector;
        unsigned char* buffer = page_address(bvec.bv_page);
        unsigned int offset = bvec.bv_offset;
        size_t len = bvec.bv_len;
        int dir = rq_data_dir(rq);
        if(dir == 1)
        {
          work_write.sector = sector;
          work_write.buffer = buffer+offset;
          work_write.len = len;
          schedule_work(&work_write.work);
          printk(KERN_INFO"Write for sector %d is scheduled\n",(int)sector);
        }
        else
        {
          work_read.sector = sector;
          work_read.buffer = buffer+offset;
          work_read.len = len;
          schedule_work(&work_read.work);
          printk(KERN_INFO"Read for sector %d is scheduled\n",(int)sector);
        }
      }
    }
    if(!__blk_end_request_cur(rq,0))
    {
      rq = blk_fetch_request(que);
    }
  }
}

/*------------------------------------------------------------------*/

static int __init block_init(void)
{
  struct file* f;
  int res;
  char* path = FILE_PATH;
  INIT_WORK(&work_write.work,block_write);
  INIT_WORK(&work_read.work,block_read);
  f = filp_open(FILE_PATH,O_RDWR,0666);
  if(f == NULL)
  {
    printk(KERN_INFO"File open error - For MBR writing\n");
    return -ENOMEM;
  }
  res = copy_mbr(f);
  filp_close(f,NULL);
  device = kmalloc(sizeof(struct my_block_struct),GFP_KERNEL);
  if( (major=register_blkdev(0,DEV_NAME))<0)
  {
    printk(KERN_INFO"Unable to register block device : dof\n");
    return -EBUSY;
  }
  printk("Major_number:%d\n",major);
  device->gd = alloc_disk(MINORS);
  if(!device->gd)
  {
    printk(KERN_INFO"Gendisk is not allocated\n");
    unregister_blkdev(major,DEV_NAME);
    kfree(device);
    return -ENOMEM;
  }
  strcpy(device->gd->disk_name,DEV_NAME);
  device->gd->first_minor = FIRST_MINOR;
  device->gd->major = major;
  device->gd->fops = &my_block_ops;
  spin_lock_init(&device->lock);
  if(!(device->gd->queue = blk_init_queue(block_request,&device->lock)))
  {
    printk("Request_queue allocated failed\n");
    del_gendisk(device->gd);
    unregister_blkdev(major,DEV_NAME);
    kfree(device);
    return -ENOMEM;
  }
  blk_queue_logical_block_size(device->gd->queue,DEV_SEC_SIZE);
  device->queue = device->gd->queue;
  device->gd->queue->queuedata = path;
  set_capacity(device->gd,(DEV_NR_SEC*DEV_SEC_SIZE/KERNEL_SEC_SIZE));
  device->gd->private_data = device;
  add_disk(device->gd);
  printk(KERN_INFO"Block device successfully registered\n");
  return 0;
}

static void __exit block_exit(void)
{
 blk_cleanup_queue(device->queue);
 del_gendisk(device->gd);
 unregister_blkdev(major,DEV_NAME);
 kfree(device);
 flush_scheduled_work();
 printk(KERN_INFO"Block device unregistered successfully\n");
}

module_init(block_init);
module_exit(block_exit);

MODULE_AUTHOR("Perumalla Deepak");
MODULE_DESCRIPTION("Block device driver");
MODULE_LICENSE("GPL");

