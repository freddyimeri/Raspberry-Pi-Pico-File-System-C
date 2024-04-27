

// /**
//  * Copies a file from the source path to the destination path, ensuring not to overwrite existing files
//  * in the destination by appending "Copy" to the file name if necessary.
//  *
//  * @param source_path The path to the source file.
//  * @param dest_path The path to the destination where the file should be copied.
//  * @return Returns 0 on success, -1 on error.
//  */
// int fs_cp(const char* source_path, const char* dest_path) {
//     // Extract the directory and filename parts from the source and destination paths.
//     PathParts old_path = extract_last_two_parts(source_path);
//     PathParts new_path = extract_last_two_parts(dest_path);


//     char* source_filename= old_path.filename;
//     char* source_directory_path= old_path.directory;
//     char* dest_directory_path= new_path.directory;
//     char* dest_filename= new_path.filename;
    
//     strcpy(dest_filename, source_filename);

//     printf("Source Filename: %s\n", source_filename);
//     printf("Source Directory: %s\n", source_directory_path);
//     printf("Destination Directory: %s\n", dest_directory_path);
//     printf("Destination Filename: %s\n", dest_filename);


//      if (dest_directory_path[0] == '\0') {
//         strcpy(dest_directory_path, "/root");  // Set default path if empty
//     }
//      if (source_directory_path[0] == '\0') {
//         strcpy(source_directory_path, "/root");  // Set default path if empty
//     }
//     printf("Source Directory: %s\n", source_directory_path);

//     DirectoryEntry* directory = DIR_find_directory_entry(source_directory_path);
//      uint32_t parentDirId = directory->currentDirId;


    
//     // Validate the modified source filename.
//     if (source_filename[0] == '\0') {
//         printf("Error: Source path '%s' does not contain a valid file name.\n", source_path);
//         return -1;
//     }
//     // Find the file entry by name in the filesystem to get the unique file ID.

//     FileEntry* entry = FILE_find_file_entry(source_filename, parentDirId);

//     if (entry == NULL) {
//         printf("Error: File '%s' not found.\n", source_filename);
//         return -1;
//     }


//     // Determine the destination directory, default to "/root" if unspecified.
//     if (dest_directory_path[0] == '\0') {
//         strcpy(dest_directory_path, "/root");  // Set default path if empty
//     } 
//     printf("Destination Directory: %s\n", dest_directory_path);
//     // Lookup the directory entry for the destination path. in order to retrieve the parent directory ID.
//     // This way we can ensure the file is copied to a valid directory with a valid parent directory ID.
//     DirectoryEntry* destDirEntry = DIR_find_directory_entry(dest_directory_path); // Store parent directory ID for later use.
//     if (!destDirEntry) {
//         printf("Error: Destination directory '%s' does not exist.\n", dest_directory_path);
//         return -1;
//     }
//     uint32_t parentIDdestination = destDirEntry->currentDirId;
//     printf("Parent ID: %u\n", parentIDdestination);
   


//     int check = find_file_existance(dest_filename, parentIDdestination);
//     if (check == 0) {
//         printf("File name already exists in the destination directory. add Copy extension \n");
//         appendCopyToFilename(dest_filename);
//         int checkDEST = find_file_existance(dest_filename, parentIDdestination);
//         if (checkDEST == 0) {
//             printf("ERROR File copy already exists in the destination directory. with name:%s \n", dest_filename);
//             return -1;
//         }
        

//     }else{
//         strcpy(dest_filename, source_filename);
//     }

//     // add the dest_directory_path + dest_filename to get the full path
//     char dest_full_path[256];
//       size_t len = strlen(dest_directory_path);
//     if (dest_directory_path[len - 1] != '/') {
//         snprintf(dest_full_path, sizeof(dest_full_path), "%s%s", dest_directory_path, dest_filename);
//     } else {
//         snprintf(dest_full_path, sizeof(dest_full_path), "%s%s", dest_directory_path, dest_filename + 1); // +1 to skip the leading slash in dest_filename
//     }

//     char source_full_path[256];
//       size_t len1 = strlen(dest_directory_path);
//     if (source_directory_path[len1 - 1] != '/') {
//         snprintf(source_full_path, sizeof(dest_full_path), "%s%s", source_directory_path, source_filename);
//     } else {
//         snprintf(source_full_path, sizeof(source_full_path), "%s%s", source_directory_path, source_filename + 1); // +1 to skip the leading slash in dest_filename
//     }


//     // Create a new file entry for the destination file.  
//     FS_FILE* fileCopy = fs_open(dest_full_path, "w"); // this will be the coppy file
//         if (fileCopy == NULL) {
//             printf("Error: Failed to open file '%s' for copying.\n", dest_filename);
//             return -1;
//         }

//     // now open the previous file to read the contents
//     FS_FILE* oldfile = fs_open(source_full_path, "r");

//     if (oldfile == NULL) {
//         printf("Error: Failed to open file '%s' for reading.\n", source_filename);
//         return -1;
//     }


//     printf("fileCopy->entry->filename: %s\n", fileCopy->entry->filename);
//     printf("fileCopy->entry->size: %d\n", fileCopy->entry->size);
//     printf("oldfile->entry->filename: %d\n", oldfile->entry->filename);

//     printf("dest_filename: %s\n", dest_filename);


//         // now we have the index of the file that we want to copy, we can now copy the contents of the old file into the new file
//     // we will copy the contents of the old file into the new file

//     printf("fileCopy->entry->filename: %s\n", fileCopy->entry->filename);
//     printf(" source_filename: %s\n", source_filename);  


   

//      // Corrected to use strlen for string copying
//     fileCopy->entry->size = oldfile->entry->size;
//     // fileCopy->entry->parentDirId = parentIDdestination; /// this one we will change
//     memcpy(fileCopy->entry->buffer, oldfile->entry->buffer, sizeof(oldfile->entry->buffer));

//     // memcpy( fileSystem[fileIndex].buffer, oldfile->entry->buffer, sizeof(oldfile->entry->buffer));

//     printf("fileCopy->entry->filename: %s\n", fileCopy->entry->filename);
//     /////////////////////////////////////

//     // calculate the offset of the file in the flash memory
//     uint32_t writeOffset = (fileCopy->entry->start_block * FILESYSTEM_BLOCK_SIZE);
//     flash_write_safe2(writeOffset, (const uint8_t*)&fileCopy->entry, sizeof(fileCopy->entry)); 
//     printf("Data written to file.\n");
//     // Clean up open file handles.
//     fs_close(oldfile);
//     fs_close(fileCopy);
//     printf("byyyyyyyyyyyyyyyyyyyeeeeeeeeeee");

//    return 0;

// }



/**
 * Opens a file with the specified mode.
 * 
 * @param FullPath The full path to the file.
 * @param mode The mode to open the file in ('r' for read, 'w' for write, 'a' for append).
 * @return A pointer to an FS_FILE structure if successful, NULL otherwise.
 */
// FS_FILE* fs_open(const char* FullPath, const char* mode) {

//     PathParts pathExtract = extract_last_two_parts(FullPath);

//     char* filename= pathExtract.filename;
//     char* directory_path= pathExtract.directory;
    
//     if (filename[0] == '\0') {
//         printf("Error: Path '%s' does not contain a valid file name.\n", FullPath);
//         fflush(stdout);
//         return NULL;
//     }

//     // checks if the directory path is empty, if so set it to root
//     set_default_path(directory_path, "/root");

//     DirectoryEntry* directory = DIR_find_directory_entry(directory_path);
//     if (directory == NULL) {
//     printf("Error: Source directory '%s' does not exist.\n", source_directory_path);
//     return -1;
//     }
//      uint32_t parentDirId = directory->currentDirId;

//     // Allocate memory for the FS_FILE structure.
//     FS_FILE* file = (FS_FILE*)malloc(sizeof(FS_FILE));
//     if (!file) {
//         printf("Error: Memory allocation failed for FS_FILE.\n");
//         fflush(stdout);
//         return NULL;
//     }

//     // Handle different modes of file operation.
//     if (strcmp(mode, "w") == 0) {
//         // Write mode: reset or create file.
//         printf("Opening file '%s' in write mode.\n", filename);
//         ///////////////////////////////////
     
//             // If the file does not exist, create a new file entry.
//             FileEntry*  entry = createFileEntry(filename, parentDirId);
//             if (entry == NULL) {
//                 printf("Error: Failed to create file entry for '%s'.\n", filename);
//                 fflush(stdout);
//                 return NULL;
            
//             }
       
//         uint32_t offsetInBytes = entry->start_block * FILESYSTEM_BLOCK_SIZE;
//         // Write the initial file entry to flash.
//         flash_write_safe2(offsetInBytes, (const uint8_t*)entry, sizeof(FileEntry));
//         file->entry = entry;
//         file->position = 0;  // Start at the beginning of the file for writing.
//         file->mode = 'w';


//     } else if (strcmp(mode, "a") == 0) {
//         FileEntry* entry = FILE_find_file_entry(filename, parentDirId);
//         // Append mode: file must exist.
//         if (entry == NULL) {
//             printf("Error: File '%s' not found.\n", filename);
//             fflush(stdout);
    
//             return NULL;
//         }
//         printf("Opening file '%s' in append mode.\n", filename);
//         file->entry = entry;
//         file->position = entry->size;  // Set the position to the end of the file to append.
//         file->mode = 'a';


//     } else if (strcmp(mode, "r") == 0) {
//         FileEntry* entry = FILE_find_file_entry(filename, parentDirId);
//         // Read mode: file must exist.
//         if (entry == NULL) {
//             printf("Error: File '%s' not found.\n", filename);
//             fflush(stdout);
   
//             return NULL;
//         }
//         printf("Opening file '%s' in read mode.\n", filename);
//         file->entry = entry;
//         file->position = 0;  // Start reading from the beginning of the file.
//         file->mode = 'r';


//     } else {
//         // Handle invalid mode.

//         printf("Error: Invalid mode '%s'.\n", mode);
//         fflush(stdout);
//         return NULL;
//     }

//     // Return the prepared FS_FILE structure.
//     return file;
// }