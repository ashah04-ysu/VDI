#include "vdi_read.h"
#include <iostream>
#include <string>
#include "vdifile.h"
#include <fstream>
#include <stack>
#include <sstream>
#include <vector>
#include "mbr.h"
#include "vdiInfo.h"
#include "vdi_write.h"

using namespace std;

/**
 * Driver for the system that copies files into and out of a vdifile in an ext2 filesystem.
 * Takes in filname as a command-line argument.
 */

int main(int argc, char* argv[])
{
	int vdiFile;
	VDIFile* file = new VDIFile;
	vdiFile = vdiOpen(file, argv[1]); //Opens the vdifile
	off_t offset;
	ssize_t numChar;
	short int magic; //For testing purposes

	/**
	* Reads the vdiheader
	*/
	offset = vdiSeek(file, 0, SEEK_SET);
	numChar = vdiRead(file, &(file->header), sizeof(file->header));

	/**
	* Reads the vdimap
	* Needed for dynamic files
	*/
	int mapChar;
	unsigned int vdiMap[file->header.blocksInHdd];
	mapChar = readVdiMap(file, vdiMap);

	/**
	* Reads the MBR (Master Boot Record)
	*/
	int numMBR;
	BootSector boot_sector;
	numMBR = readMbr(file, boot_sector);
	if (numMBR == 1) return 1;

	/**
	* Reads the superblock
	*/
	ext2_super_block super_block;
	int numSuperBlock;
	numSuperBlock = readSuperblock(file, boot_sector, vdiMap, super_block);
	if (numSuperBlock == 1) return 1;

	/**
	* Reads the group descriptor
	*/
	unsigned int group_count = (super_block.s_blocks_count - super_block.s_first_data_block) / super_block.
		s_blocks_per_group;
	unsigned int remainder = (super_block.s_blocks_count - super_block.s_first_data_block) % super_block.
		s_blocks_per_group;
	if (remainder > 0) group_count++;
	unsigned int block_size = 1024 << super_block.s_log_block_size;
	ext2_group_descriptor group_descriptor[group_count];
	if (readGroupDescriptor(file, boot_sector, vdiMap, block_size, group_descriptor, group_count) == 1) return 1;

	/**
	 * Reads the inode
	 */
	ext2_inode inode = readInode(file, boot_sector, vdiMap, 2, block_size, super_block, group_descriptor);

	/**
	* Fetching directories from the inode
	*/
	string path = "Current path: /";
	string instr = "Instruction: ";
	unsigned char* buf = (unsigned char*)malloc(block_size);
	ext2_dir_entry_2 current; //Serves as the current directory
	bool found = false;

	unsigned int root_size = inode.size / block_size; //size of the root inode
	if (inode.size % block_size > 0) root_size++;

	for (int i = 0; i < root_size; i++)
	{
		int size_difference;
		size_difference = readBlock(inode, i, block_size, file, boot_sector, vdiMap, buf);
		if (size_difference == -1)
		{
			cout << "The filesystem couldn't be displayed! - 1" << endl;
			return 1;
		}
		if (getDirEntry(current, buf, size_difference, ".", false))
		{
			//This condition will meet only if the directory entry can be located.
			found = true;
			break;
		}
	}
	if (!found)
	{
		//If the directory cannot be located for some reason
		cout << "The file system couldn't be displayed - 2" << endl;
		return 1;
	}

	//Need to display the contents here without help being enabled. So, basically something like a quick demo
	cout << "\033[2J\033[1;1H" << endl;
	cout << "VDI COPY SYSTEM" << endl;
	cout << "################################################################################" << endl;
	cout << "# This program can copy files into and out of a vdifile in an ext2 filesystem. 	#" << endl;;
	cout << "# It supports UNIX commands like 'ls', 'cd', 'cd ..', 'read', and 'write'. 			#" << endl;
	cout << "################################################################################" << endl;
	cout << endl;
	cout << "================================================================================" << endl;
	cout << "To change a directory -- cd [dirname]" << endl;
	cout << "To move one directory up -- cd .." << endl;
	cout << "To display files within the current directory -- ls" << endl;
	cout << "To view the individual parts of a vdi file -- view" << endl;
	cout << "To copy files from the vdifile into host location -- read [ext2path/filename] [user_path/filename] [NOTE: paths should be in this format: /examples/foo.txt]" << endl;
	cout << "To copy files into the vdifile from host location -- write [user_path/filename] [ext2path/filename] [NOTE: paths should be in this format: /examples/foo.txt]" << endl;
	cout << "To quit -- quit" << endl;
	cout << "================================================================================" << endl;
	cout << endl;
	cout << instr << endl;

	stack<int>* name_dir_length = new stack<int>; //Stores the lengths for the directories' names
	bool loop = true;
	string instruction; //Stores the command entered by the user

	while (loop)
	{
		getline(cin, instruction);
		/**
		* This is basically used for debugging, testing purposes.
		* But, if the user wants to view specific details about the structure, can use this.
		*/
		if (instruction == "view")
		{
			display_vdihead(file);
			display_vdimap(vdiMap);
			display_MBR(boot_sector);
			display_superblock(super_block);
			display_inode(inode);
		}
			/**
			* Moves the current directory to one level up (just like UNIX)
			*/
		else if (instruction == "cd ..")
		{
			if (current.inode != 2)
			{
				//Means you're not in root
				ext2_inode inode = readInode(file, boot_sector, vdiMap, current.inode, block_size, super_block, group_descriptor);
				char fname[256];
				memcpy(fname, current.name, current.name_len);
				//copies the name_length block of data from the current dir's name to the filename
				fname[current.name_len] = '\0'; //Appending null character to the filename
				if ((string)fname != "..") name_dir_length->push((int)current.name_len);
				unsigned int f_size = inode.size / block_size; //calculates file size
				if (inode.size % block_size > 0) f_size++;
				found = false;
				for (int i = 0; i < f_size; i++)
				{
					int size_difference;
					size_difference = readBlock(inode, i, block_size, file, boot_sector, vdiMap, buf);
					if (size_difference == -1)
					{
						cout << "The filesystem couldn't be displayed! -3" << endl;
						return 1;
					}
					if (getDirEntry(current, buf, size_difference, "..", false))
					{
						found = true;
						break;
					}
				}
				if (!found) cout << "The directory couldnt be located." << endl;
				else
				{
					int bound_1 = path.length();
					int bound_2 = name_dir_length->top();
					int bound_final = bound_1 - bound_2 - 1;
					path = path.substr(0, bound_final); //New path is calculated
					name_dir_length->pop();
				}
			}
			else cout << "You are at the root already!" << endl;
			cout << path << endl;
		}
			/**
			* Lists the contents of the directory.
			*/
		else if (instruction == "ls")
		{
			ext2_inode new_inode = readInode(file, boot_sector, vdiMap, current.inode, block_size, super_block,
			                                  group_descriptor);
			unsigned int file_size = inode.size / block_size;
			if (inode.size % block_size > 0) file_size++;
			for (int i = 0; i < file_size; i++)
			{
				int size_difference;
				size_difference = readBlock(new_inode, i, block_size, file, boot_sector, vdiMap, buf);
				if (size_difference == -1)
				{
					cout << "Error displaying the file!" << endl;
					return 1;
				}
				getDirEntry(current, buf, size_difference, "", true);
			}
		}
			/**
			* Moves to the specified directory (UNIX like)
			* P.S. - There is no auto-complete by hitting tab like UNIX, so user need to type in the entire dir name.
			*/
		else if (instruction.compare(0, 3, "cd ") == 0) //Need to find a better way than compare if any
		{
			ext2_inode new_inode_2 = readInode(file, boot_sector, vdiMap, current.inode, block_size, super_block,
			                                    group_descriptor);
			string dir = instruction.substr(3, instruction.length() - 1); //Extracting the dir name
			char fname[256];
			memcpy(fname, current.name, current.name_len);
			if ((string)fname != "..") name_dir_length->push((int)current.name_len);
			ext2_dir_entry_2 new_dir; //creating a new dir for the source dir
			unsigned int f_size = new_inode_2.size / block_size;
			if (new_inode_2.size % block_size > 0) f_size++;
			found = false;
			for (int i = 0; i < f_size; i++)
			{
				int size_difference;
				size_difference = readBlock(new_inode_2, i, block_size, file, boot_sector, vdiMap, buf);
				if (size_difference == -1)
				{
					cout << "The filesystem couldn't be displayed! - 4" << endl;
					return 1;
				}
				if (getDirEntry(new_dir, buf, size_difference, dir, false))
				{
					found = true;
					break;
				}
			}
			if (!found) cout << "The directory cannot be located!" << endl;
			else
			{
				if ((int)new_dir.file_type == 2)
				{
					//File_type = 2 means it is a directory.
					/**
					file_type			Description
					0							Unknown
					1							Regular File
					2							Directory
					3							Character Device
					4							Block Device
					5							Named pipe
					6							Socket
					7							Symbolic Link
					*/
					current = new_dir;
					path += dir + "/";
				}
				else cout << "It is not a directory!" << endl;
			}
		}
			/**
			* Copies file from the vdifile to the specified host location.
			*/
		else if (instruction.compare(0, 4, "read") == 0)
		{
			//String parsing for path. Need to think of a better way to parse string here.
			stringstream ss(instruction);
			vector<string> elements;
			string item;
			while (getline(ss, item, ' ')) elements.push_back(item);
			vector<string> split1 = elements; //Basically, this is just splitting the path from the instruction
			if (split1.size() < 3)
			{
				cout << "Error, there are fewer arguments than required!" << endl;
				continue;
			}
			string ext2path = split1[1]; //Separating the paths
			string user_path = split1[2];
			stringstream ss2(ext2path);
			vector<string> elements2;
			string item2;
			while (getline(ss2, item2, '/')) elements2.push_back(item2);
			vector<string> split2 = elements2; //Extracting the filename from the path
			ext2_inode current_inode = readInode(file, boot_sector, vdiMap, current.inode, block_size, super_block,
			                                      group_descriptor); //Creating a new current inode
			ext2_dir_entry_2 curr_dir; //Creates a new current directory to work on
			if (split2.size() < 2)
			{
				cout << "The path for ext2 is not in a correct format!" << endl;
				continue;
			}
			found = false;
			for (int j = 1; j < split2.size(); j++)
			{
				unsigned int fsize = current_inode.size / block_size;
				if (current_inode.size % block_size > 0) fsize++;
				for (int i = 0; i < fsize; i++)
				{
					int size_difference;
					size_difference = readBlock(current_inode, i, block_size, file, boot_sector, vdiMap, buf);
					if (size_difference == -1)
					{
						cout << "The path for the directory could not be parsed!" << endl;
						return 1;
					}
					if (getDirEntry(curr_dir, buf, size_difference, split2[j], false))
					{
						//split2[j] will have the dir name where it should navigate to
						found = true;
						break;
					}
				}
				if (!found)
				{
					cout << "The directory could not be found in ext2path!" << endl;
					return 1;
				}
				else
				{
					current_inode = readInode(file, boot_sector, vdiMap, curr_dir.inode, block_size, super_block,
					                           group_descriptor);
				}
			}
			if (user_path.empty())
			{
				cout << "Please enter a path in your host system." << endl;
				continue;
			}
			int fd = open(user_path.c_str(), O_CREAT | O_WRONLY | O_APPEND, S_IRWXU);
			if (fd < 0)
			{
				cout << "The file can not be created in the host system in the given user path!" << endl;
				continue;
			}
			if ((int)curr_dir.file_type != 1)
			{
				//It is not a regular file
				cout << "The file in the ext2path cannot be read" << endl;
				continue;
			}
			unsigned int file_size = current_inode.size / block_size;
			if (current_inode.size % block_size > 0) file_size++;
			for (int i = 0; i < file_size; i++)
			{
				int size_difference = readBlock(current_inode, i, block_size, file, boot_sector, vdiMap, buf);
				if (size_difference == -1)
				{
					cout << "The path of the directory could not be parsed! " << endl;
					return 1;
				}
				if (i + 1 == file_size) write(fd, buf, size_difference); //Forgot why I did this, but there WAS A REASON!!!
				else write(fd, buf, block_size);
			}
			cout << "The file " + ext2path + " has been copied to " + user_path << endl;
			close(fd);
		}
			/**
			* Copies the file from the host location to the vdifile.
			*/
		else if (instruction.compare(0, 5, "write") == 0) //Need to find a better way
		{
			cout << endl;
			//String parsing for path
			vector<string> elements1;
			stringstream ss(instruction);
			string item;
			while (getline(ss, item, ' ')) elements1.push_back(item);
			vector<string> split1 = elements1; //Again just splitting path from instruction
			if (split1.size() < 3)
			{
				cout << "Not enough arguments to carry out the action." << endl;
				continue;
			}
			string user_path = split1[1]; //Separating paths
			string ext2path = split1[2];

			vector<string> elements2;
			stringstream ss2(ext2path);
			string item2;
			while (getline(ss2, item2, '/')) elements2.push_back(item2);
			vector<string> split2 = elements2; //Getting the dir name
			if (split2.empty() || ext2path.compare(0, 1, "/") != 0)
			{
				cout << "The ext2 path is not in a correct format" << endl;
				continue;
			}
			if (split2[split2.size() - 1].empty())
			{
				//Filename not provided
				cout << "Please provide a file name." << endl;
				continue;
			}
			int fd = open(user_path.c_str(), O_RDONLY);
			if (fd < 0)
			{
				cout << "Cannot open the file in the given user path!" << endl;
				continue;
			}
			ext2_inode current_inode = readInode(file, boot_sector, vdiMap, current.inode, block_size, super_block,
			                                      group_descriptor);
			ext2_dir_entry_2 current_dir; //Creating the new current dir to navigate to
			found = false;
			for (int i = 0; i < root_size; i++)
			{
				int size_difference;
				size_difference = readBlock(inode, i, block_size, file, boot_sector, vdiMap, buf);
				if (size_difference == -1)
				{
					cout << "The file system can't be displayed! - 5" << endl;
					return 1;
				}
				if (getDirEntry(current_dir, buf, size_difference, ".", false))
				{
					//get the entry for root dir
					found = true;
					break;
				}
			}
			if (!found)
			{
				cout << "The current dir can not be set to the root dir" << endl;
				return -1;
			}
			found = false;
			for (int j = 1; j < split2.size() - 1; j++)
			{
				unsigned int size = current_inode.size / block_size;
				if (current_inode.size % block_size > 0) size++;
				for (int k = 0; k < size; k++)
				{
					int size_difference;
					size_difference = readBlock(current_inode, k, block_size, file, boot_sector, vdiMap, buf);
					if (size_difference == -1)
					{
						cout << "The dir path could not be parsed!" << endl;
						return 1;
					}
					bool val = getDirEntry(current_dir, buf, size_difference, split2[j], false);
					//Get the entry for the directory where we want to copy the file
					if (val)
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					cout << "The dir entry in ext2path could not be located!" << endl;
					return 1;
				}
				else
					current_inode = readInode(file, boot_sector, vdiMap, current_dir.inode, block_size, super_block,
					                           group_descriptor);
			}
			string name = split2[split2.size() - 1]; //Name of the copied file in the ext2 path.
			unsigned int new_record_len = 8 + name.length() + 1; //setting the directory record length
			if (new_record_len % 4 > 0) new_record_len += 4 - (new_record_len % 4);
			unsigned int dir_size = current_inode.size / block_size;
			if (current_inode.size % block_size > 0) dir_size++;
			unsigned char* last_dir_block = (unsigned char*)malloc(block_size);
			int size_difference;
			size_difference = readBlock(current_inode, dir_size - 1, block_size, file, boot_sector, vdiMap, last_dir_block);
			if (size_difference == -1)
			{
				cout << "The directory path can not be parsed!" << endl;
				return 1;
			}
			/**
			 * I will document the following block of code later.
			 */
			unsigned int blocks_for_dir_entry;
			if (block_size - size_difference >= new_record_len) blocks_for_dir_entry = 0;
			else
			{
				int new_direct, old_direct, new_indirect, old_indirect, new_double, old_double, new_triple, old_triple;
				computeIndex(dir_size, block_size, new_direct, new_indirect, new_double, new_triple);
				computeIndex(dir_size - 1, block_size, old_direct, old_indirect, old_double, old_triple);
				if (new_triple != -1 && old_triple == -1) blocks_for_dir_entry = 4;
				else if (new_triple != old_triple) blocks_for_dir_entry = 3;
				else if (new_double != -1 && old_double == -1) blocks_for_dir_entry = 3;
				else if (new_double != old_double) blocks_for_dir_entry = 2;
				else if (new_indirect != -1 && old_indirect == -1) blocks_for_dir_entry = 2;
				else if (new_indirect != old_indirect) blocks_for_dir_entry = 1;
				else blocks_for_dir_entry = 1;
			}
			unsigned int f_size = lseek(fd, 0, SEEK_END) + 1;
			unsigned int f_block_num = f_size / block_size;
			if (f_size % block_size > 0) f_block_num++;
			unsigned int free_block_num = super_block.s_free_blocks_count - super_block.s_r_blocks_count;
			int direct, indirect, double_, triple;
			computeIndex(f_block_num - 1, block_size, direct, indirect, double_, triple);
			unsigned int blocks_for_addresses;
			if (direct != -1 && indirect == -1 && double_ == -1 && triple == -1) blocks_for_addresses = 0;
			else if (direct == -1 && indirect != -1 && double_ == -1 && triple == -1) blocks_for_addresses = 1;
			else if (direct == -1 && indirect != -1 && double_ != -1 && triple == -1)
				blocks_for_addresses = 1 + double_ + 1
					+ 1;
			else if (direct == -1 && indirect != -1 && double_ != -1 && triple != -1)
				blocks_for_addresses = 1 + triple + 1
					+ triple * (block_size / 4) + double_ + 1 + 1 + (block_size / 4) + 1;

			if (f_block_num + blocks_for_addresses + blocks_for_dir_entry > free_block_num)
			{
				cout << "The file cannot be fit" << endl;
				return 1;
			}

			if (super_block.s_free_inodes_count <= 0)
			{
				cout << "There isn't enough space to write a new inode" << endl;
				return 1;
			}

			unsigned int inode_address = 0;
			int i = 0;
			for (i = 0; i < group_count; i++)
			{
				unsigned char* bitmap = readBitmap(block_size, group_descriptor[i].inode_bitmap, file, boot_sector, vdiMap);
				unsigned int inode_offset = i * super_block.s_inodes_per_group;
				if (isInodeFree(bitmap, block_size, inode_offset, inode_address))
				{
					if (writeBitmap(file, boot_sector, vdiMap, bitmap, block_size, group_descriptor[i].inode_bitmap) == 1)
					{
						cout << "Writing the inode bitmap failed " << endl;
						return 1;
					}
					group_descriptor[i].free_inodes_count--;
					break;
				}
			}
			vector<unsigned int> addresses;
			for (i; i < f_block_num + blocks_for_addresses + blocks_for_dir_entry; i++)
			{
				unsigned int address;
				for (int j = 0; j < group_count; j++)
				{
					unsigned char* bitmap = readBitmap(block_size, group_descriptor[j].block_bitmap, file, boot_sector, vdiMap);
					unsigned int block_offset = j * super_block.s_blocks_per_group;
					if (isBlockFree(bitmap, block_size, block_offset, address))
					{
						addresses.push_back(address);
						if (writeBitmap(file, boot_sector, vdiMap, bitmap, block_size, group_descriptor[j].block_bitmap) == 1)
						{
							cout << "Writing the block bitmap failed" << endl;
							return 1;
						}
						group_descriptor[j].free_blocks_count--;
						break;
					}
				}
			}
			ext2_inode new_inode = readInode(file, boot_sector, vdiMap, 12, block_size, super_block, group_descriptor);
			new_inode.mode = 0x8000 | 0x0100 | 0x0080 | 0x0040;
			new_inode.uid = 1000;
			new_inode.size = f_size;
			new_inode.gid = 1000;
			new_inode.links_count = 1;
			new_inode.blocks = f_size / 512;
			if (f_size % 512 > 0) new_inode.blocks++;
			if (direct != -1 && indirect == -1 && double_ == -1 && triple == -1)
			{
				for (i = 0; i <= 14; i++)
				{
					if (i < addresses.size())
					{
						new_inode.i_block[i] = addresses.back();
						addresses.pop_back();
					}
					else new_inode.i_block[i] = 0;
				}
			}
			else if (direct == -1 && indirect != -1 && double_ == -1 && triple == -1)
			{
				for (i = 0; i <= 14; i++)
				{
					if (i <= 12)
					{
						new_inode.i_block[i] = addresses.back();
						addresses.pop_back();
					}
					else new_inode.i_block[i] = 0;
				}
			}
			else if (direct == -1 && indirect != -1 && double_ != -1 && triple == -1)
			{
				for (i = 0; i <= 13; i++)
				{
					new_inode.i_block[i] = addresses.back();
					addresses.pop_back();
				}
				new_inode.i_block[14] = 0;
			}
			else if (direct == -1 && indirect != -1 && double_ != -1 && triple != -1)
			{
				for (i = 0; i <= 14; i++)
				{
					new_inode.i_block[i] = addresses.back();
					addresses.pop_back();
				}
			}
			for (i = 0; i < f_block_num; i++)
			{
				if (lseek(fd, i * block_size, SEEK_SET) < 0)
				{
					cout << "The host file cannot be seeked" << endl;
					return 1;
				}
				if (read(fd, buf, block_size) < 0)
				{
					cout << "Reading a block of data from user file has failed" << endl;
					return 1;
				}
				int size_difference = writeBlock(file, boot_sector, vdiMap, new_inode, addresses, i, block_size, buf);
				if (size_difference == -1)
				{
					cout << "Writing a block to the ext2 filesystem failed" << endl;
					return 1;
				}
			}
			if (writeInode(file, boot_sector, vdiMap, new_inode, inode_address, block_size, super_block,
			                group_descriptor) == 1)
			{
				cout << "Writing inode has failed." << endl;
				return 1;
			}
			ext2_dir_entry_2 new_dir_entry;
			new_dir_entry.inode = inode_address;
			new_dir_entry.name_len = name.length() + 1;
			new_dir_entry.rec_len = new_record_len;
			new_dir_entry.file_type = 1;
			memcpy(new_dir_entry.name, name.c_str(), name.size());
			new_dir_entry.name[name.length()] = '\0';
			current_inode.size += new_dir_entry.rec_len;
			vector<unsigned int> empty;
			if (block_size - size_difference >= new_dir_entry.rec_len)
			{
				memcpy(last_dir_block + size_difference, &new_dir_entry, new_dir_entry.rec_len);
				if (writeBlock(file, boot_sector, vdiMap, current_inode, empty, dir_size - 1, block_size,
				                last_dir_block) == -1)
				{
					cout << "Writing the data block has failed." << endl;
					return 1;
				}
			}
			else
			{
				unsigned char* new_block = (unsigned char*)malloc(block_size);
				memset(new_block, 0, block_size);
				memcpy(new_block, &new_dir_entry, new_dir_entry.rec_len);
				int new_direct_num, old_direct_num;
				int new_indirect_index, old_indirect_index;
				int new_double_index, old_double_index;
				int new_triple_index, old_triple_index;

				computeIndex(dir_size, block_size, new_direct_num, new_indirect_index, new_double_index, new_triple_index);
				computeIndex(dir_size - 1, block_size, old_direct_num, old_indirect_index, old_double_index,
				              old_triple_index);
				if (new_triple_index != -1 && old_triple_index == -1)
				{
					current_inode.i_block[14] = addresses.back();
					addresses.pop_back();
				}
				else if (new_double_index != -1 && old_double_index == -1 && new_triple_index == -1)
				{
					current_inode.i_block[13] = addresses.back();
					addresses.pop_back();
				}
				else if (new_indirect_index != -1 && old_indirect_index == -1 && new_double_index == -1)
				{
					current_inode.i_block[12] = addresses.back();
					addresses.pop_back();
				}
				else if (new_direct_num != old_direct_num)
				{
					current_inode.i_block[new_direct_num] = addresses.back();
					addresses.pop_back();
				}

				if (writeBlock(file, boot_sector, vdiMap, current_inode, addresses, dir_size, block_size, new_block) == -1)
				{
					cout << "Witing the data block has failed" << endl;
					return 1;
				}
				free(new_block);
			}
			free(last_dir_block);

			if (!addresses.empty())
			{
				cout << "The allocated addresses were not completely used" << endl;
				return 1;
			}

			if (writeInode(file, boot_sector, vdiMap, current_inode, current_dir.inode, block_size, super_block,
			                group_descriptor) == 1)
			{
				cout << "Writing the dir inode has failed" << endl;
				return 1;
			}
			if (writeGroupDescriptor(file, boot_sector, vdiMap, block_size, group_descriptor, group_count) == 1)
			{
				cout << "Writing the group descriptor has failed." << endl;
				return 1;
			}

			super_block.s_free_blocks_count -= (f_block_num + blocks_for_addresses + blocks_for_dir_entry);
			super_block.s_free_inodes_count--;

			if (writeSuperblock(file, boot_sector, vdiMap, super_block) == 1)
			{
				cout << "Writing the super block has failed." << endl;
				return 1;
			}

			cout << "File " + user_path + " has been copied to " + ext2path << endl;
		}
		else if (instruction == "quit")
		{
			loop = false;
		}
		cout << endl;
		cout << "================================================================================" << endl;
		cout << path << endl;
	}
	close(file->file);
	free(buf);
	return 0;
}
