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
                    print('INVALID ' + str(blockFd) + ' ' + str(location[i]) + ' IN INODE ' + str(list_for_node[j][0]) + ' AT OFFSET ' + str(buff))
                if location[i]<first_block:
                    error_fd = True
                    fd_2 = False
                    print('RESERVED ' + str(blockFd) + ' ' + str(location[i]) + ' IN INODE ' + str(list_for_node[j][0]) + ' AT OFFSET ' + str(buff))
                if(fd_2):
                    if location[i] not in hashmap:
                        hashmap[location[i]] = []
                        hashmap[location[i]].append(block_list_1)
                    else:
                        hashmap[location[i]].append(block_list_1)
            else:
                error_fd = True
                print('ALLOCATED BLOCK ' + str(location[i]) + ' ON FREELIST')

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
                print('INVALID ' + str(blockFd) + ' ' + str(indirect_list[i][4]) + ' IN INODE ' + str(indirect_list[i][0]) + ' AT OFFSET ' + str(buff))
            if indirect_list[i][4]<first_block:
                error_fd = True
                print('RESERVED ' + str(blockFd) + ' ' + str(indirect_list[i][4]) + ' IN INODE ' + str(indirect_list[i][0]) + ' AT OFFSET ' + str(buff))
        else:
            error_fd = True
            print('ALLOCATED BLOCK ' + str(indirect_list[i][4]) + '  ON FREELIST')

    for key in hashmap:
        if len(hashmap[key])>1:
            for i, temp in enumerate(hashmap[key]):
                print('DUPLICATE ' + str(hashmap[key][i][0]) + ' ' + str(hashmap[key][i][1]) + ' IN INODE ' + str(hashmap[key][i][2]) + ' AT OFFSET ' + str(hashmap[key][i][3]))

    for k in range(first_block,list_of_lists[1]):
        if k not in cfree_list and k not in hashmap:
            error_fd = True
            print('UNREFERENCED BLOCK ' + str(k))
    
    afree_list = dfree_list

    for j, node2 in enumerate(list_for_node):
        #print(node2[0])
        #print(nodeCount)
        nodeCount = node2[0]
        if list_for_node[j][0] != '0':
            bfree_list.append(node2)
            if nodeCount in dfree_list:
                print('ALLOCATED INODE '+ str(nodeCount) + ' ON FREELIST')
                error_fd = True
                afree_list.remove(nodeCount)
        else:
            if nodeCount not in dfree_list:
                print('UNALLOCATED INODE ' + str(nodeCount) + '  NOT ON FREELIST')
                error_fd = True
                afree_list.append(nodeCount)

    begin = sb_list[6]
    end = sb_list[1]
    for nodeIndex in range(begin,end):
        storageList = []
        for j, node2 in enumerate(list_for_node):
            if list_for_node[j][0] == nodeIndex:
                storageList.append(node2)
        if len(storageList) <= 0 and (nodeIndex not in dfree_list):
            print('UNALLOCATED INODE ' + str(nodeIndex) + ' NOT ON FREELIST')
            error_fd = True
            afree_list.append(nodeIndex)

    node_d = {}

    for i, directory in enumerate(dirent_list):
        nodeCount = dirent_list[i][2]
        directory = dirent_list[i][5][:-1]
        parentCount = dirent_list[i][0]
        if nodeCount in afree_list:
            print('DIRECTORY INODE ' + str(parentCount) + ' NAME ' + str(directory) + ' UNALLOCATED INODE ' + str(nodeCount))
            error_fd = True
        elif nodeCount < 1 or nodeCount > sb_list[1]:
            print('DIRECTORY INODE ' + str(parentCount) + ' NAME ' + str(directory) + ' INVALID INODE ' + str(nodeCount))
            error_fd = True
        else:
            if (nodeCount not in placeholder):
                placeholder[nodeCount] = dirent_list[i][0]
            if nodeCount not in node_d:
                node_d[nodeCount] = 1
            else:
                node_d[nodeCount] = 1 + node_d[nodeCount]

        if directory == "'.'":
            if nodeCount != parentCount:
                error_fd = True
                print('DIRECTORY INODE ' + str(parentCount) +  " NAME '.' LINK TO INODE " + str(nodeCount) + ' SHOULD BE ' + str(parentCount))
        if directory == "'..'":
            if placeholder[parentCount] != nodeCount:
                error_fd = True
                print('DIRECTORY INODE ' + str(parentCount) +  " NAME '..' LINK TO INODE " + str(nodeCount) + ' SHOULD BE ' + str(placeholder[parentCount]))

    for node in bfree_list:
        #print(node)
        if node[0] not in node_d:
            if node[5] != 0:
                error_fd = True
                print('INODE ' + str(node[0]) + ' HAS 0 LINKS BUT LINKCOUNT IS ' + str(node[5]))
        else:
            linkCount = node[5]
            linkJoin = node_d[node[0]]
            if linkJoin != linkCount:
                error_fd = True
                print('INODE ' + str(node[0]) + ' HAS ' + str(linkJoin) + ' LINKS BUT LINKCOUNT IS ' + str(linkCount))
    if error_fd:
        sys.exit(2)
    else:
        sys.exit(0)

    
if __name__ == "__main__":
    main()
