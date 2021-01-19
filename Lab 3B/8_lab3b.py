import sys
import csv 

def main():
    #setup vars
    file_fd = ""
    list_store = []
    error_fd = False
    afree_list = []
    bfree_list = []
    placeholder = {2:2}

    sb_list = []
    list_of_lists = []
    cfree_list = []
    dfree_list = []
    list_for_node = []
    dirent_list = []
    indirect_list = []
    hashmap = {}
    
    #check input and begin extracting csv
    if len(sys.argv) != 2:
        sys.stderr.write("Error: Incorrect argumetns\n")
        sys.exit(1)
    file_fd = sys.argv[1]
    try:
        with open(file_fd,'r') as ins:
            for row in ins:
                list_store.append(row.split(","))
    except:
        sys.stderr.write("Error: Failed to extract csv\n")
        sys.exit(1)

    #iterate through data to setup lists
    for row in list_store:
        if row[0] == 'SUPERBLOCK':
            sb_list.append(int(row[1]))
            sb_list.append(int(row[2]))
            sb_list.append(int(row[3]))
            sb_list.append(int(row[4]))
            sb_list.append(int(row[5]))
            sb_list.append(int(row[6]))
            sb_list.append(int(row[7]))
        elif row[0] == 'GROUP':
            list_of_lists.append(int(row[1]))
            list_of_lists.append(int(row[2]))
            list_of_lists.append(int(row[3]))
            list_of_lists.append(int(row[4]))
            list_of_lists.append(int(row[5]))
            list_of_lists.append(int(row[6]))
            list_of_lists.append(int(row[7]))
            list_of_lists.append(int(row[8]))
        elif row[0] =='BFREE':
            cfree_list.append(int(row[1]))
        elif row[0] == 'IFREE':
            dfree_list.append(int(row[1]))
        elif row[0] == 'INODE':
            list_of_nodes2 = [int(row[1]),row[2],int(row[3]),int(row[4]),int(row[5]),int(row[6]),row[7],row[8],row[9],int(row[10]),int(row[11]),list(map(int,row[12:]))]
            list_for_node.append(list_of_nodes2)
        elif row[0] == 'DIRENT':
            list_of_direct = [int(row[1]),int(row[2]),int(row[3]),int(row[4]),int(row[5]),row[6]]
            dirent_list.append(list_of_direct)
        elif row[0] == 'INDIRECT':
            list_of_indirect = [int(row[1]),int(row[2]),int(row[3]),int(row[4]),int(row[5])]
            indirect_list.append(list_of_indirect)
        else:
            sys.stderr.write("Error: Input Data Unreadable\n")
            sys.exit(1)
                        
    target = sb_list[0]
    first_block = list_of_lists[7]+int(sb_list[3]*list_of_lists[2]/sb_list[2])
    for j,node in enumerate(list_for_node):
        #print(list_for_node[j][11])
        location = list_for_node[j][11]
        if list_for_node[j][10] == 0 and list_for_node[j][1] == 's':
            continue
        for i in range(len(location)):
            if location[i]==0:
                continue
            blockFd = ""
            buff = 0
            if i == 12:
                blockFd = "INDIRECT BLOCK"
                buff = 12
            elif i == 13:
                buff = 268
                blockFd = "DOUBLE INDIRECT BLOCK"
            elif i==14:
                buff=65804
                blockFd = "TRIPLE INDIRECT BLOCK"
            else:
                blockFd = "BLOCK"

            block_list_1 = [blockFd,location[i],list_for_node[j][0],buff]

            if location[i] not in cfree_list:
                fd_2 = True
                if location[i]<0 or location[i]>target:
                    error_fd = True
                    fd_2 = False
                    print('INVALID {} {} IN INODE {} AT OFFSET {}'.format(blockFd,location[i],list_for_node[j][0],buff))
                if location[i]<first_block:
                    error_fd = True
                    fd_2 = False
                    print('RESERVED {} {} IN INODE {} AT OFFSET {}'.format(blockFd,location[i],list_for_node[j][0],buff))
                if(fd_2):
                    if location[i] not in hashmap:
                        hashmap[location[i]] = []
                        hashmap[location[i]].append(block_list_1)
                    else:
                        hashmap[location[i]].append(block_list_1)
            else:
                error_fd = True
                print('ALLOCATED BLOCK {} ON FREELIST'.format(location[i]))

    for i, indirect in enumerate(indirect_list):
        blockFd = ""
        buff = indirect_list[i][2]

        if indirect_list[i][1] == 1:
            blockFd = "INDIRECT BLOCK"
        elif indirect_list[i][1] == 2:
            blockFd = "DOUBLE INDIRECT BLOCK"
        elif indirect_list[i][1] == 3:
            blockFd = "TRIPLE INDIRECT BLOCK"
        else:
            blockFd = "BLOCK"


        block_list_1 = [blockFd,indirect_list[i][4],indirect_list[i][0],buff]

        if indirect_list[i][4] not in cfree_list:
            if indirect_list[i][4] not in hashmap:
                hashmap[indirect_list[i][4]] = []
                hashmap[indirect_list[i][4]].append(block_list_1)
            else:
                hashmap[indirect_list[i][4]].append(block_list_1)
            if int(indirect_list[i][4])<0 or int(indirect_list[i][4])>target:
                error_fd = True
                print('INVALID {} {} IN INODE {} AT OFFSET {}'.format(blockFd,indirect_list[i][4],indirect_list[i][0],buff))
            if indirect_list[i][4]<first_block:
                error_fd = True
                print('RESERVED {} {} IN INODE {} AT OFFSET {}'.format(blockFd,indirect_list[i][4],indirect_list[i][0],buff))
        else:
            error_fd = True
            print('ALLOCATED BLOCK {} ON FREELIST'.format(indirect_list[i][4]))

    for key in hashmap:
        if len(hashmap[key])>1:
            for i, temp in enumerate(hashmap[key]):
                print('DUPLICATE {} {} IN INODE {} AT OFFSET {}'.format(hashmap[key][i][0],hashmap[key][i][1],hashmap[key][i][2],hashmap[key][i][3]))


    for k in range(first_block,list_of_lists[1]):
        if k not in cfree_list and k not in hashmap:
            error_fd = True
            print('UNREFERENCED BLOCK {}'.format(k))
    
    afree_list = dfree_list

    for j, node2 in enumerate(list_for_node):
        #print(node2[0])
        #print(node_number)
        node_num = node2[0]
        if list_for_node[j][0] != '0':
            bfree_list.append(node2)
            if node_num in dfree_list:
                print("ALLOCATED INODE %d ON FREELIST" % node_num)
                error_fd = True
                afree_list.remove(node_num)
        else:
            if node_num not in dfree_list:
                print("UNALLOCATED INODE %d NOT ON FREELIST" % node_num)
                error_fd = True
                afree_list.append(node_num)

    start = sb_list[6]
    end = sb_list[1]
    for node_pos in range(start,end):
        reserved_list = []
        for j, node2 in enumerate(list_for_node):
            if list_for_node[j][0] == node_pos:
                reserved_list.append(node2)
        if len(reserved_list) <= 0 and (node_pos not in dfree_list):
            print("UNALLOCATED INODE %d NOT ON FREELIST" % node_pos)
            error_fd = True
            afree_list.append(node_pos)

    node_d = {}

    for i, directory in enumerate(dirent_list):
        node_num = dirent_list[i][2]
        dir_name = dirent_list[i][5][:-1]
        parent_num = dirent_list[i][0]
        if node_num in afree_list:
            print('DIRECTORY INODE {} NAME {} UNALLOCATED INODE {}'.format(parent_num, dir_name,node_num))
            error_fd = True
        elif node_num < 1 or node_num > sb_list[1]:
            print('DIRECTORY INODE {} NAME {} INVALID INODE {}'.format(parent_num, dir_name,node_num))
            error_fd = True
        else:
            if (node_num not in placeholder):
                placeholder[node_num] = dirent_list[i][0]
            if node_num not in node_d:
                node_d[node_num] = 1
            else:
                node_d[node_num] = 1 + node_d[node_num]

        if dir_name == "'.'":
            if node_num != parent_num:
                error_fd = True
                print("DIRECTORY INODE %d NAME '.' LINK TO INODE %d SHOULD BE %d" % (parent_num,node_num,parent_num))
        if dir_name == "'..'":
            if placeholder[parent_num] != node_num:
                error_fd = True
                print("DIRECTORY INODE %d NAME '..' LINK TO INODE %d SHOULD BE %d" % (parent_num,node_num,placeholder[parent_num]))

    for node in bfree_list:
        #print(node)
        if node[0] not in node_d:
            if node[5] != 0:
                error_fd = True
                print('INODE {} HAS 0 LINKS BUT LINKCOUNT IS {}'.format(node[0],node[5]))
        else:
            link_node = node[5]
            link_entries = node_d[node[0]]
            if link_entries != link_node:
                error_fd = True
                print('INODE {} HAS {} LINKS BUT LINKCOUNT IS {}'.format(node[0],link_entries,link_node))
    if error_fd:
        sys.exit(2)
    else:
        sys.exit(0)

    
if __name__ == "__main__":
    main()
