import csv
import sys

filename = ""
data = []
error_flag = False

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
        self.block_addresses = l


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

class block:
    def __init__(self,a,b,c,d):
        self.block_type = a
        self.block_num = b
        self.inode_num = c
        self.offset = d

sp = superblock(0,0,0,0,0,0,0)
group_list = []
bfree_list = []
ifree_list = []
inode_list = []
directory_list = []
indirect_list = []

block_map = {}
def block_consistency_audit():
    global error_flag
    global sp
    max_size = sp.block_per_group
    first_block = group_list[0].first_block_inode 
    for inode in inode_list:
        addresses = inode.block_addresses
        for i in range(len(addresses)):
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
            

            temp_block = block(block_type,addresses[i],inode.inode_number,offset)

            if addresses[i] not in bfree_list:
                if addresses[i] not in block_map:
                    block_map[addresses[i]] = []
                    block_map[addresses[i]].append(temp_block)
                else:
                    block_map.append(temp_block)
                if addresses[i]<0 or addresses[i]>max_size:
                    error_flag = True
                    print('INVALID {} {} IN INODE {} AT OFFSET {}'.format(block_type,addresses[i],inode.inode_number,offset))
                if addresses[i]<first_block:
                    error_flag = True
                    print('RESERVED {} {} IN INODE {} AT OFFSET {}'.format(block_type,addresses[i],inode.inode_number,offset))
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
        

        temp_block = block(block_type,indirect.blocknum_referenced,indirect.owning_file,offset)

        if indirect.blocknum_referenced not in bfree_list:
            if  not in block_map:
                block_map[indirect.blocknum_reference] = []
                block_map[indirect.blocknum_reference].append(temp_block)
            else:
                block_map.append(temp_block)
            if indirect.blocknum_reference<0 or indirect.blocknum_reference>max_size:
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
                print('DUPLICATE {} {} IN INODE AT {} AT OFFSET {}'.format(temp.block_type,temp.block_num,temp.inode_num,temp.offset))


    for k in range(first_block,group_list[0].block_number_in_group):
        if k not in bfree_list and k not in block_map:
            error_flag = True
            print('UNREFERENCED BLOCK {}'.format(k))
    

    

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

    if error_flag:
        sys.exit(2)
    else:
        sys.exit(0)
if __name__ == "__main__":
    main()
