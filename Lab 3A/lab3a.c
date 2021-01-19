#include <stdio.h>
#include <errno.h>
#include <limits.h> 
#include <getopt.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>
#include "ext2_fs.h"

int main(int argc, char **argv) {

	int fd;

	struct ext2_super_block superBlock;
	struct ext2_inode inode;
	struct ext2_group_desc group_desc;
	struct ext2_dir_entry dir;
	    
	uint32_t block_len = 0;
	uint32_t inode_len = 0;
	uint32_t  file_len = 0;
	char file_buff; 

	int temp, indirect_a, indirect_b, indirect_c;
	uint32_t  k = 0, m = 0;

	if (argc != 2) {
                fprintf(stderr, "Error: incorrect arguments\n");
                exit(1);
	} 

	struct option long_options[] = {
		{0, 0, 0, 0}
	};

	if (getopt_long(argc, argv, "", long_options, 0) != -1) {
		fprintf(stderr, "Error: Incorrect arguments\n");
		exit(1);
	}

	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		fprintf(stderr, "Error: Failed to  mount\n");
		exit(2);
	}

	if (pread(fd, &superBlock, sizeof(struct ext2_super_block), 1024) < 0) {
		fprintf(stderr, "Error: Failed to pread\n");
		exit(1);
	}


	block_len = EXT2_MIN_BLOCK_SIZE << superBlock.s_log_block_size;
	inode_len = superBlock.s_inode_size;
	pread(fd, &superBlock, sizeof(superBlock), 1024);

	fprintf(stdout, "SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", superBlock.s_blocks_count, superBlock.s_inodes_count, block_len, inode_len, superBlock.s_blocks_per_group, superBlock.s_inodes_per_group, superBlock.s_first_ino);

	if (pread(fd, &group_desc, sizeof(struct ext2_group_desc), 1024 + sizeof(struct ext2_super_block)) < 0) {
		fprintf(stderr, "Error: Failed to pread\n");
		exit(1);
	}

	fprintf(stdout, "GROUP,%d,%u,%u,%u,%u,%u,%u,%u\n", 0, superBlock.s_blocks_count, superBlock.s_inodes_count, group_desc.bg_free_blocks_count, group_desc.bg_free_inodes_count, group_desc.bg_block_bitmap, group_desc.bg_inode_bitmap, group_desc.bg_inode_table);
	uint32_t i = 0;
	while (i < superBlock.s_blocks_count) {
		int bit = i & 7;
		uint8_t bitmap = i >> 3;
		unsigned char parse;
		if (pread(fd, &parse, sizeof(uint8_t), group_desc.bg_block_bitmap * block_len + bitmap) < 0) {
		    fprintf(stderr, "Error: Failed to pread\n");
		    exit(1);
		}
		if (!((parse >> bit) & 1)) {
		    fprintf(stdout, "BFREE,%d\n", i + 1);
		}
		i++;
	}
	i = 0;
	while (i < superBlock.s_inodes_count) {
		int bit = i & 7;
		uint8_t bitmap = i >> 3;
		unsigned char parse;

		if (pread(fd, &parse, sizeof(uint8_t), group_desc.bg_inode_bitmap * block_len + bitmap) < 0) {
		    fprintf(stderr, "Error: Failed to pread\n");
		    exit(1);
		}

		if (!((parse >> bit) & 1)) {
		    fprintf(stdout, "IFREE,%d\n", i + 1);
		}
		i++;
	}

	i = 0; 
	uint32_t j = 0;
	file_buff = 0;
	while (i < superBlock.s_inodes_count) {
		int bit = i & 7;
		unsigned char parse;
		if (pread(fd, &parse, sizeof(uint8_t), group_desc.bg_inode_bitmap * block_len + (uint8_t)(i >> 3)) < 0){   
		    fprintf(stderr, "Error: Failed to pread\n");
		    exit(1);
		}
		if (!((parse >> bit) & 1)) {   
		    i++;
		    continue;
		}

		if (pread(fd, &inode, inode_len, 1024 + (group_desc.bg_inode_table - 1) * (block_len) + i * sizeof(struct ext2_inode)) < 0) {   
		    fprintf(stderr, "Error: Failed to pread\n");
		    exit(1);
		}

		if (!(inode.i_mode) || !(inode.i_links_count)) {   
		    i++;
		    continue;
		}

		if ((inode.i_mode & 0xF000) == 0xA000) 
			file_buff = 's';
		else if ((inode.i_mode & 0xF000) ==  0x8000)  
			file_buff = 'f';
		else if ((inode.i_mode & 0xF000) == 0x4000) { 
		    file_buff = 'd'; 
		    uint32_t buffer = 1024 + (group_desc.bg_inode_table - 1) * (block_len) + i * sizeof(struct ext2_inode);
		    int num = i + 1;

			j = 0; 
			for(j = 0; j < 12; j++) {
				if (pread(fd, &temp, 4, buffer + 40 + j * 4) < 0) {
				    fprintf(stderr, "Error: Failed to pread\n");
				    exit(1);
				}

				if (temp) {
				    int current_buffer = block_len * temp;
				    uint32_t curr = current_buffer;
				    while (curr < current_buffer + block_len) {
					    if (pread(fd, &dir, sizeof(struct ext2_dir_entry), curr) < 0) {
						fprintf(stderr, "Error: Failed to pread\n");
						exit(1);
					    }
					    uint32_t buff_2 = curr - current_buffer;
					    uint32_t inode_count = dir.inode;
					    uint32_t intialization = dir.rec_len;
					    curr += intialization;
					    uint32_t title_len = dir.name_len;
					    if (!inode_count)
					    continue;
					    fprintf(stdout, "DIRENT,%u,%u,%u,%u,%u,'%s'\n", num, buff_2, inode_count, intialization, title_len, dir.name);
				    }
				}
			}


			if (pread(fd, &indirect_a, 4, buffer + 40 + 48) < 0) {
				fprintf(stderr, "Error: Failed to pread\n");
				exit(1);
			}
			if (indirect_a) {
				for (j = 0; j < block_len / 4; j++) {
				    if (pread(fd, &temp, 4, block_len * indirect_a + j * 4) < 0) {
					fprintf(stderr, "Error: Failed to pread\n");
					exit(1);
				    }
				    int current_buffer = block_len * temp;
				    if (temp) {
					uint32_t curr = current_buffer;
					while (curr < current_buffer + block_len) {
					if (pread(fd, &dir, sizeof(struct ext2_dir_entry), curr) < 0) {
					   fprintf(stderr, "Error: Failed to pread\n");
					   exit(1);
					}
					uint32_t buff_2 = curr - current_buffer;
					uint32_t inode_count = dir.inode;
					uint32_t intialization = dir.rec_len;
					curr += intialization;
					uint32_t title_len = dir.name_len;
					if (!inode_count)
					    continue;
					fprintf(stdout, "DIRENT,%u,%u,%u,%u,%u,'%s'\n", num, buff_2, inode_count, intialization, title_len, dir.name);
				    }
					}
				}
			}


			if (pread(fd, &indirect_b, 4, buffer + 40 + 52) < 0) {
				fprintf(stderr, "Error: Failed to pread\n");
				exit(1);
			}
			if (indirect_b) {
				for (k = 0; k < block_len / 4; k++) {
				    if (pread(fd, &indirect_a, 4, indirect_b * block_len + k * 4) < 0) {
					fprintf(stderr, "Error: Failed to pread\n");
					exit(1);
				    }
				    if (indirect_a) {
					for (j = 0; j < block_len / 4; j++) {
					    if (pread(fd, &temp, 4, block_len * indirect_a + j * 4) < 0) {   
						fprintf(stderr, "Error: Failed to pread\n");
						exit(1);
					    }
					    int current_buffer = block_len * temp;
					    if (temp) {   
						       uint32_t curr = current_buffer;
							while (curr < current_buffer + block_len) {
							if (pread(fd, &dir, sizeof(struct ext2_dir_entry), curr) < 0) {
							   fprintf(stderr, "Error: Failed to pread\n");
							   exit(1);
							}
							uint32_t buff_2 = curr - current_buffer;
							uint32_t inode_count = dir.inode;
							uint32_t intialization = dir.rec_len;
							curr += intialization;
							uint32_t title_len = dir.name_len;
							if (!inode_count)
							    continue;
							fprintf(stdout, "DIRENT,%u,%u,%u,%u,%u,'%s'\n", num, buff_2, inode_count, intialization, title_len, dir.name);
						    }   
						}
					}
				    }
			}
			}

			if (pread(fd, &indirect_c, 4, buffer + 40 + 56) < 0) {   
				fprintf(stderr, "Error: Failed to pread\n");
				exit(1);
			}
			if (indirect_c) {   
				
				for (m = 0; m < block_len / 4; m++) {   
				    if (pread(fd, &indirect_b, 4, indirect_c * block_len + m * 4) < 0) {   
					fprintf(stderr, "Error: Failed to pread\n");
					exit(1);
				    }
				    if (indirect_b) {  
					for (k = 0; k < block_len / 4; k++) {   
					    if (pread(fd, &indirect_a, 4, indirect_b * block_len + k * 4) < 0) {   
						fprintf(stderr, "Error: Failed to pread\n");
						exit(1);
					    }
					    if (indirect_a != 0) {   
						for (j = 0; j < block_len / 4; j++) {   
						    if (pread(fd, &temp, 4, block_len * indirect_a + j * 4) < 0) {   
							fprintf(stderr, "Error: Failed to pread\n");
							exit(1);
						    }
						    int current_buffer = block_len * temp;
						    if (temp) {   
							       uint32_t curr = current_buffer;
								while (curr < current_buffer + block_len) {
									if (pread(fd, &dir, sizeof(struct ext2_dir_entry), curr) < 0) {
									   fprintf(stderr, "Error: Failed to pread\n");
									   exit(1);
									}
									uint32_t buff_2 = curr - current_buffer;
									uint32_t inode_count = dir.inode;
									uint32_t intialization = dir.rec_len;
									curr += intialization;
									uint32_t title_len = dir.name_len;
									if (!inode_count)
									    continue;
									fprintf(stdout, "DIRENT,%u,%u,%u,%u,%u,'%s'\n", num, buff_2, inode_count, intialization, title_len, dir.name);
							    }
							}
						}
					    }
					}
				    }
				}
				}
			}
		

		fprintf(stdout, "INODE,%d,%c,%o,%u,%u,%u", i + 1, file_buff, (inode.i_mode & 0xFFF), inode.i_uid, inode.i_gid, inode.i_links_count); 

		char *ctime = "a";
		time_t timer1 = inode.i_ctime;
		struct tm *timer2 = gmtime(&timer1);
		char *timer3 = malloc(sizeof(char) * 32);
		strftime(timer3, 32, "%m/%d/%y %H:%M:%S", timer2);
		ctime =  timer3; 

		char *mtime = "a"; 

		timer1 = inode.i_mtime;
		timer2 = gmtime(&timer1);
		char *timer32 = malloc(sizeof(char) * 32);
		strftime(timer32, 32, "%m/%d/%y %H:%M:%S", timer2);
		mtime =  timer32;

		char *atime = "a";

		timer1 = inode.i_atime;
		timer2 = gmtime(&timer1);
		char *timer33 = malloc(sizeof(char) * 32);
		strftime(timer33, 32, "%m/%d/%y %H:%M:%S", timer2);
		atime =  timer33;


		file_len = inode.i_size; 
		fprintf(stdout, ",%s,%s,%s,%d,%d", ctime, mtime, atime, file_len, inode.i_blocks);


		if (!(file_buff == 's' && file_len < 60)) {
		    j = 0;
		    while (j < 15) {
			fprintf(stdout, ",%u", inode.i_block[j]);
			j++;
		    }
		} else {
		    fprintf(stdout, ",%u", inode.i_block[0]);
		}

		fprintf(stdout, "\n");

		uint32_t inode_count = i + 1;


		if (!(file_buff == 's' && file_len < 60)) {
		    if (inode.i_block[12]) {
			    uint32_t block_buffer = inode.i_block[12] * block_len;
			    uint32_t curr_block, i = 0;
			    while (i < block_len / 4) {   
				if (pread(fd, &curr_block, sizeof(curr_block), block_buffer + i * 4) < 0) {   
				    fprintf(stderr, "Error: Failed to pread\n");
				    exit(1);
				}
				if (curr_block) {   
				    fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", inode_count, 1, 12 + i, inode.i_block[12], curr_block);
				}
				i++;
			    }
		    }
		    if (inode.i_block[13]) {
			    uint32_t indirect_buff = inode.i_block[12 + 1] * block_len;
			    uint32_t indirect_curr_block, curr_block, block_buffer;
			    uint32_t i = 0, j = 0;
			    while (i < block_len / 4) {
				if (pread(fd, &indirect_curr_block, sizeof(curr_block), indirect_buff + i * 4) < 0) {
				    fprintf(stderr, "Error: Failed to pread\n");
				    exit(1);
				}
				block_buffer = indirect_curr_block * block_len;
				if (indirect_curr_block) {
				    fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", inode_count, 2, 12 + block_len / 4 + i * block_len / 4, inode.i_block[12 + 1], indirect_curr_block);
				    while (j < block_len / 4) {
					if (pread(fd, &curr_block, sizeof(curr_block), block_buffer + j * 4) < 0) {
					    fprintf(stderr, "Error: Failed to pread\n");
					    exit(1);
					}
					if (curr_block) {
					    fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", inode_count, 1, 12 + block_len / 4 + i * block_len / 4 + j, indirect_curr_block, curr_block);
					}
					j++;
				    }
				}
				i++;
			    }
		    }
		    if (inode.i_block[14]) {
			    uint32_t triple_buff = inode.i_block[12 + 2] * block_len;
			    uint32_t block_parsed = 12 + block_len / 4 + block_len / 4 * block_len / 4;
			    uint32_t tuple_buff, single_buff, indirect_double, indirect_single;
			    uint32_t i = 0, j = 0, k = 0;

			    while (i < block_len / 4) {
				if (pread(fd, &indirect_double, sizeof(indirect_double), triple_buff + i * 4) < 0) {
				    fprintf(stderr, "Error: Failed to pread\n");
				    exit(1);
				}
				tuple_buff = indirect_double * block_len;
				if (indirect_double) {
				    fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", inode_count, 3, block_parsed + i * (block_len * block_len / 8), inode.i_block[12 + 2], indirect_double);
				    while (j < block_len / 4) {
					if (pread(fd, &indirect_single, sizeof(indirect_single), tuple_buff + j * 4) < 0) {
					    fprintf(stderr, "Error: Failed to pread\n");
					    exit(1);
					}
					single_buff = indirect_single * block_len;
					if (indirect_single) {
					    fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", inode_count, 2, block_parsed + i * (block_len * block_len / 8) + j * (block_len / 4), indirect_double, indirect_single);
					    while (k < block_len / 4) {
						uint32_t block_counter2;
						if (pread(fd, &block_counter2, sizeof(block_counter2), single_buff + k * 4) < 0) {
						    fprintf(stderr, "Error: Failed to pread\n");
						    exit(1);
						}
						if (block_counter2)
						    fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", inode_count, 1, block_parsed + i * (block_len * block_len / 8) + j * (block_len / 4) + k, indirect_single, block_counter2);
						k++;
					    }
					}
					j++;
				    }
				}
				i++;
			    }

		    }
		}
		i++;
	}


	}

