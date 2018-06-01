import csv
import sys

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
        self.blocknum_referenced = e

sp = superblock(0,0,0,0,0,0,0)
group_list = []
bfree_list = []
ifree_list = []
inode_list = []
directory_list = []
indirect_list = []


def block_consistency_audit():
    

def inode_allocation_audits():
    global inode_list, error_flag, ifree_list, unallocated_inodes, allocated_inodes
    unallocated_inodes = ifree_list
    for inode in inode_list:
        inode_num = inode.inode_number
        if inode.file_type != '0':
            if inode_num in ifree_list:
                print("ALLOCATED INODE %d ON FREELIST" % inode_num)
                error_flag = True
                allocated_inodes.append(inode)
                unallocated_inodes.remove(inode_num)
        else:
            if inode_num not in ifree_list:
                print("UNALLOCATED INODE %d NOT ON FREELIST" % inode_num)
                error_flag = True
                unallocated_inodes.append(inode_num)
    #
    # edit free inode lists?
    #
def find_parent():
    global inode_parent, directory_list
    for directory in directory_list:
        inode_num = directory.reference_inode
        if directory.name != "'.'" and directory.name != "'..'":
            if inode_num >= 1 and inode_num <= sp.total_num_inode and inode_num in allocated_inodes:
                inode_parent[inode_num] = directory.parent_inode_number


def directory_consistency_audits():
    global directory_list, unallocated_inodes, allocated_inodes, inode_parent
    inode_d = {}
    find_parent()
    for directory in directory_list:
        inode_num = directory.reference_inode
        dir_name = directory.name
        parent_num = directory.parent_inode_number
        if inode_num in unallocated_inodes:
            print("DIRECTORY INODE %d NAME %s UNALLOCATED INODE %d" % (parent_num, dir_name, inode_num))
            error_flag = True
        else if inode_num < 1 or inode_num > sp.total_num_inode:
            print("DIRECTORY INODE %d NAME %s INVALID INODE %d" % (parent_num, dir_name, inode_num))
            error_flag = True
        else if inode_num not in inode_d
            inode_d[inode_num] = 1
        else
            inode_d[inode_num] = 1 + inode_d[inode_num]

        if dir_name == "'.'":
            if inode_num != parent_num:
                error_flag = True
                print("DIRECTORY INODE %d NAME '.' LINK TO INODE %d SHOULD BE %d" % (parent_num, inode_num,parent_num))
        if dir_name == "'..'":
            if inode_parent[parent_num] != inode_num:
                error_flag = True
                print("DIRECTORY INODE %d NAME '..' LINK TO INODE %d SHOULD BE %d" % (parent_num, inode_num,inode_parent[parent_num]))
                
    for inode in allocated_inodes:
        if inode not in inode_d:
            if inode.link_count != 0:
                error_flag = True
                print("INODE %d HAS 0 LINKS BUT LINKCOUNT IS %d" % (inode.inode_number,inode.link_count))
        else:
            link_inode = inode.link_count
            link_entries = inode_d[inode.inode_number]
            if link_entries != link_inode:
                error_flag = True
                print("INODE %d HAS %d LINKS BUT LINKCOUNT IS %d" % (inode.inode_number,link_entries,link_inode))

def main():
    if len(sys.args)!=2:
        print >> sys.stderr,"invalid number of arguments"
        sys.exit(1)
    filename = sys.args[1]
    try:
        with open(filename,'r') as ins:
            for line in ins:
                data.append(line.split(","))
    except:
        print>>sys.stderr,"cannot open csv file"
        sys.exit(1)
    for line in data:
        if line[0] == 'SUPERBLOCK':
            global sp=superblock(int(line[1]),int(line[2]),int(line[3]),int(line[4]),int(line[5]),int(line[6]),int(line[7]))
        elif line[0]=='GROUP':
            temp_group = group(int(line[1]),int(line[2]),int(line[3]),int(line[4]),int(line[5]),int(line[6]),int(line[7]),int(line[8]))
            group_list.append(temp_group)
        elif line[0] =='BFREE':
            bfree_list.append(int(line[1]))
        elif line[0] == 'IFREE':
            ifree_list.append(int(line[1]))
        elif line[0] == 'INODE':
            temp_inode = inode(int(line[1]),line[2],int(line[3]),int(line[4]),int(line[5]),int(line[6]),line[7],line[8],line[9],int(line[10]),int(line[11]),line[12:])
            inode_list.append(temp_inode)
        elif line[0] == 'DIRENT':
            temp_direc = directory(int(line[1]),int(line[2]),int(line[3]),int(line[4]),int(line[5]),line[6])
            directory_list.append(temp_direc)
        elif line[0] == 'INDIRECT':
            temp_indirect = indirect(int(line[1]),int(line[2]),int(line[3]),int(line[4]),int(line[5]))
            indirect_list.append(temp_indirect)
        else:
            print >> sys.stderr, "invalid data line"
            sys.exit(1)
                        

    block_consistency_audit()
    inode_allocation_audits()
    directory_consistency_audits()


    
if __name__ == "__main__":
    main()
