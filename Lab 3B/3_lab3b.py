import sys
import csv 

filename = ""
data = []
error_flag = False
unallocated_inodes = []
allocated_inodes = []
inode_parent = {2:2}

class superblock:
    def __init__(self,a,b,c,d,e,f,g): 
        self.total_num_blocks=a
        self.total_num_inode=b
        self.block_size=c
        self.inode_size=d
        self.block_per_group=e
        self.inode_per_group=f
        self.first_nonreserved_inode=g

class group:
    def __init__(self,a,b,c,d,e,f,g,h):
        self.group_number=a
        self.block_number_in_group=b
        self.inode_number_in_group=c
        self.freeblock_number=d
        self.freeinode_number=e
        self.freeblock_bitmap_num=f
        self.freeinode_bitmap_num=g
        self.first_block_inode=h

class inode:
    def __init__(self,a,b,c,d,e,f,g,h,i,j,k,l):
        self.inode_number = a
        self.file_type = b
        self.mode = c
        self.owner = d
        self.group = e
        self.link_count = f
        self.ctime = g
        self.mtime = h
        self.atime = i
        self.file_size = j
        self.disk_space = k
        self.block_address = l


class directory:
    def __init__(self,a,b,c,d,e,f):
        self.parent_inode_number = a
        self.logical_offset = b
        self.reference_inode = c
        self.entry_length = d
        self.name_length = e
        self.name = f


class indirect:
    def __init__(self,a,b,c,d,e):
        self.owning_file = a
        self.level = b
        self.logical_offset = c
        self.blocknum_scanned = d
        self.blocknum_reference = e

sp = superblock(0,0,0,0,0,0,0)
group_list = []
bfree_list = []
ifree_list = []
inode_list = []
directory_list = []
indirect_list = []

class block:
    def __init__(self,a,b,c,d):
        self.block_type = a
        self.block_num = b
        self.inode_num = c
        self.offset = d

block_map = {}
def block_consistency_audit():
    global error_flag
    global sp
    max_size = sp.total_num_blocks
    first_block = group_list[7]+int(sp.inode_size*group_list[2]/sp.block_size)

    addresses = inode_list[11]
    #if inode_list[10] == 0 and inode_list[1] == 's':
    #    continue
    for i in range(len(addresses)):
        if inode_list[10] == 0 and inode_list[1] == 's':
            continue
        if addresses[i]==0:
            continue
        block_type = ""
        offset = 0
        if i == 12:
            block_type = "INDIRECT BLOCK"
            offset = 12
        elif i == 13:
            offset = 268
            block_type = "DOUBLE INDIRECT BLOCK"
        elif i==14:
            offset=65804
            block_type = "TRIPLE INDIRECT BLOCK"
        else:
            block_type = "BLOCK"


        temp_block = block(block_type,addresses[i],inode_list[0],offset)

        if addresses[i] not in bfree_list:
            temp_flag = True
            if addresses[i]<0 or addresses[i]>max_size:
                error_flag = True
                temp_flag = False
                print('INVALID {} {} IN INODE {} AT OFFSET {}'.format(block_type,addresses[i],inode_list[0],offset))
            if addresses[i]<first_block:
                error_flag = True
                temp_flag = False
                print('RESERVED {} {} IN INODE {} AT OFFSET {}'.format(block_type,addresses[i],inode_list[0],offset))
            if(temp_flag):
                if addresses[i] not in block_map:
                    block_map[addresses[i]] = []
                    block_map[addresses[i]].append(temp_block)
                else:
                    block_map[addresses[i]].append(temp_block)
        else:
            error_flag = True
            print('ALLOCATED BLOCK {} ON FREELIST'.format(addresses[i]))

    for indirect in indirect_list:
        block_type = ""
        offset = indirect.logical_offset

        if indirect.level == 1:
            block_type = "INDIRECT BLOCK"
        elif indirect.level == 2:
            block_type = "DOUBLE INDIRECT BLOCK"
        elif indirect.level==3:
            block_type = "TRIPLE INDIRECT BLOCK"
        else:
            block_type = "BLOCK"
        

        temp_block = block(block_type,indirect.blocknum_reference,indirect.owning_file,offset)

        if indirect.blocknum_reference not in bfree_list:
            if indirect.blocknum_reference not in block_map:
                block_map[indirect.blocknum_reference] = []
                block_map[indirect.blocknum_reference].append(temp_block)
            else:
                block_map[indirect.blocknum_reference].append(temp_block)
            if int(indirect.blocknum_reference)<0 or int(indirect.blocknum_reference)>max_size:
                error_flag = True
                print('INVALID {} {} IN INODE {} AT OFFSET {}'.format(block_type,indirect.blocknum_reference,indirect.owning_file,offset))
            if indirect.blocknum_reference<first_block:
                error_flag = True
                print('RESERVED {} {} IN INODE {} AT OFFSET {}'.format(block_type,indirect.blocknum_reference,indirect.owning_file,offset))
        else:
            error_flag = True
            print('ALLOCATED BLOCK {} ON FREELIST'.format(indirect.blocknum_reference))

    for key in block_map:
        if len(block_map[key])>1:
            for temp in block_map[key]:
                print('DUPLICATE {} {} IN INODE {} AT OFFSET {}'.format(temp.block_type,temp.block_num,temp.inode_num,temp.offset))


    for k in range(first_block,group_list[1]):
        if k not in bfree_list and k not in block_map:
            error_flag = True
            print('UNREFERENCED BLOCK {}'.format(k))
    

def inode_allocation_audits():
    global inode_list, error_flag, ifree_list, unallocated_inodes, allocated_inodes
    unallocated_inodes = ifree_list
    for inode in inode_list:
        inode_num = inode_list[0]
        if inode_list[1] != '0':
            allocated_inodes.append(inode)
            if inode_num in ifree_list:
                print("ALLOCATED INODE %d ON FREELIST" % inode_num)
                error_flag = True
                unallocated_inodes.remove(inode_num)
        else:
            if inode_num not in ifree_list:
                print("UNALLOCATED INODE %d NOT ON FREELIST" % inode_num)
                error_flag = True
                unallocated_inodes.append(inode_num)

    start = sp.first_nonreserved_inode
    end = sp.total_num_inode
    for inode_pos in range(start,end):
        reserved_list = []
        for inode in inode_list:
            if inode_list[0] == inode_pos:
                reserved_list.append(inode)
        if len(reserved_list) <= 0 and (inode_pos not in ifree_list):
            print("UNALLOCATED INODE %d NOT ON FREELIST" % inode_pos)
            error_flag = True
            unallocated_inodes.append(inode_pos)



def directory_consistency_audits():
    global directory_list, unallocated_inodes, allocated_inodes, inode_parent, error_flag
    inode_d = {}

    for directory in directory_list:
        inode_num = directory.reference_inode
        dir_name = directory.name[:-1]
        parent_num = directory.parent_inode_number
        if inode_num in unallocated_inodes:
            print('DIRECTORY INODE {} NAME {} UNALLOCATED INODE {}'.format(parent_num, dir_name,inode_num))
            error_flag = True
        elif inode_num < 1 or inode_num > sp.total_num_inode:
            print('DIRECTORY INODE {} NAME {} INVALID INODE {}'.format(parent_num, dir_name,inode_num))
            error_flag = True
        else:
            if (inode_num not in inode_parent):
                inode_parent[inode_num] = directory.parent_inode_number
            if inode_num not in inode_d:
                inode_d[inode_num] = 1
            else:
                inode_d[inode_num] = 1 + inode_d[inode_num]

        if dir_name == "'.'":
            if inode_num != parent_num:
                error_flag = True
                print("DIRECTORY INODE %d NAME '.' LINK TO INODE %d SHOULD BE %d" % (parent_num,inode_num,parent_num))
        if dir_name == "'..'":
            if inode_parent[parent_num] != inode_num:
                error_flag = True
                print("DIRECTORY INODE %d NAME '..' LINK TO INODE %d SHOULD BE %d" % (parent_num,inode_num,inode_parent[parent_num]))
                
    for inode in allocated_inodes:
        if inode_list[0] not in inode_d:
            if inode_list[5] != 0:
                error_flag = True
                print('INODE {} HAS 0 LINKS BUT LINKCOUNT IS {}'.format(inode_list[0],inode_list[5]))
        else:
            link_inode = inode_list[5]
            link_entries = inode_d[inode_list[0]]
            if link_entries != link_inode:
                error_flag = True
                print('INODE {} HAS {} LINKS BUT LINKCOUNT IS {}'.format(inode_list[0],link_entries,link_inode))

def main():
    global sp
    if len(sys.argv)!=2:
        sys.stderr.write("invalid number of arguments\n")
        sys.exit(1)
    filename = sys.argv[1]
    try:
        with open(filename,'r') as ins:
            for line in ins:
                data.append(line.split(","))
    except:
        sys.stderr.write("cannot open csv file\n")
        sys.exit(1)
    for line in data:
        if line[0] == 'SUPERBLOCK':
            sp = superblock(int(line[1]),int(line[2]),int(line[3]),int(line[4]),int(line[5]),int(line[6]),int(line[7]))
        elif line[0]=='GROUP':
            #temp_group = group(int(line[1]),int(line[2]),int(line[3]),int(line[4]),int(line[5]),int(line[6]),int(line[7]),int(line[8]))
            #group_list.append(temp_group)
            #jugg = [] 
            group_number = int(line[1])
            block_number_in_group = int(line[2])
            inode_number_in_group= int(line[3])
            freeblock_number = int(line[4])
            freeinode_number = int(line[5])
            freeblock_bitmap_num = int(line[6])
            freeinode_bitmap_num = int(line[7])
            first_block_inode = int(line[8])
            group_list.append(group_number)
            group_list.append(block_number_in_group)
            group_list.append(inode_number_in_group)
            group_list.append(freeblock_number)
            group_list.append(freeinode_number)
            group_list.append(freeblock_bitmap_num)
            group_list.append(freeinode_bitmap_num)
            group_list.append(first_block_inode)
            #temp_group = group(int(line[1]),int(line[2]),int(line[3]),int(line[4]),int(line[5]),int(line[6]),int(line[7]),int(line[8]))
            #group_list.append(jugg)
        elif line[0] =='BFREE':
            bfree_list.append(int(line[1]))
        elif line[0] == 'IFREE':
            ifree_list.append(int(line[1]))
        elif line[0] == 'INODE':
            inode_number = int(line[1])
            file_type = line[2]
            mode = int(line[3])
            owner = int(line[4])
            group = int(line[5])
            link_count = int(line[6])
            ctime = line[7]
            mtime = line[8]
            atime = line[9]
            file_size = int(line[10])
            disk_space = int(line[11])
            block_address = list(map(int,line[12:]))
            #temp_inode = inode(int(line[1]),line[2],int(line[3]),int(line[4]),int(line[5]),int(line[6]),line[7],line[8],line[9],int(line[10]),int(line[11]),list(map(int,line[12:])))
            #inode_list.append(temp_inode)
            inode_list.append(inode_number)
            inode_list.append(file_type)
            inode_list.append(mode)
            inode_list.append(owner)
            inode_list.append(group)
            inode_list.append(link_count)
            inode_list.append(ctime)
            inode_list.append(mtime)
            inode_list.append(atime)
            inode_list.append(file_size)
            inode_list.append(disk_space)
            inode_list.append(block_address)
        elif line[0] == 'DIRENT':
            temp_direc = directory(int(line[1]),int(line[2]),int(line[3]),int(line[4]),int(line[5]),line[6])
            directory_list.append(temp_direc)
        elif line[0] == 'INDIRECT':
            temp_indirect = indirect(int(line[1]),int(line[2]),int(line[3]),int(line[4]),int(line[5]))
            indirect_list.append(temp_indirect)
        else:
            sys.stderr.write("invalid data line\n")
            sys.exit(1)
                        

    block_consistency_audit()
    inode_allocation_audits()
    directory_consistency_audits()
    if error_flag:
        sys.exit(2)
    else:
        sys.exit(0)

    
if __name__ == "__main__":
    main()
