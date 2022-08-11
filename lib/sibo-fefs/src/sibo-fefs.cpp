#include "sibo-fefs.h"

int count_files = 0, count_dirs = 0;
const char* slash = "/\0";

struct tm psidateptime (const struct PsiDateTime datetime) {
    struct tm tm;

    tm.tm_year = (datetime.psi_date >> 9) + 80;
    tm.tm_mon  = ((datetime.psi_date >> 5) % 0x10) - 1;
    tm.tm_mday = datetime.psi_date % 0x20;
    tm.tm_hour = (datetime.psi_time >> 11);
    tm.tm_min  = (datetime.psi_time >> 5) % 0x40;
    tm.tm_sec  = (datetime.psi_time % 0x20) * 2;

    return(tm);
}

struct PsiDateTime psidateftime(const struct tm tm) {
    struct PsiDateTime datetime;
    
    datetime.psi_date = 0x200 * (tm.tm_year - 80) + 0x20 * (tm.tm_mon + 1) + tm.tm_mday;
    datetime.psi_time = 0x800 * tm.tm_hour + 0x20 * tm.tm_min + tm.tm_sec / 2; 

    return(datetime);
}

char *rtrim(char *s) {
	char *p = s + strlen(s) - 1;
	const char *end = p;
	while (p >= s && isspace((unsigned char) *p)) {
		p--;
	}
	if (p != end) {
        p++;
		*p = '\0';
    }
	return s;
}


// void getfile(int pos, char path[], char *buffer[], const char localpath[], const time_t unixtime, const long buffer_len) {
//     FILE *fp;
//     unsigned int cur_data_ptr = 0, cur_data_len = 0;
//     unsigned int cur_pos = pos, next_pos = 0, file_len = 0;
//     char file_flags;
//     unsigned int entry_count = 0;
//     struct utimbuf ubuf;

//     // printlogf(2, "File scanning...\n");

//     memcpy(&file_flags, *buffer + (cur_pos + FILE_FLAGS_OFFSET), FILE_FLAGS_LENGTH);

//     remove(localpath);
//     fp = fopen(localpath, "wb");

//     while (1) {
//         entry_count++;
//         // printlogf(2, "Entry %d:\n", entry_count);
//         if (entry_count == 1) {
//             memcpy(&cur_data_ptr, *buffer + (cur_pos + ENTRY_FIRSTDATARECORDPTR_OFFSET), ENTRY_FIRSTDATARECORDPTR_LENGTH);
//             memcpy(&cur_data_len, *buffer + (cur_pos + ENTRY_FIRSTDATALEN_OFFSET), ENTRY_FIRSTDATALEN_LENGTH);
//             memcpy(&file_flags, *buffer + (cur_pos + ENTRY_FLAGS_OFFSET), ENTRY_FLAGS_LENGTH);
//             memcpy(&next_pos, *buffer + (cur_pos + ENTRY_FIRSTENTRYRECORDPTR_OFFSET), ENTRY_FIRSTENTRYRECORDPTR_LENGTH);
//         } else {
//             memcpy(&cur_data_ptr, *buffer + (cur_pos + FILE_DATARECORDPTR_OFFSET), FILE_DATARECORDPTR_LENGTH);
//             memcpy(&cur_data_len, *buffer + (cur_pos + FILE_DATARECORDLEN_OFFSET), FILE_DATARECORDLEN_LENGTH);
//             memcpy(&file_flags, *buffer + (cur_pos + FILE_FLAGS_OFFSET), FILE_FLAGS_LENGTH);
//             memcpy(&next_pos, *buffer + (cur_pos + FILE_NEXTRECORDPTR_OFFSET), FILE_NEXTRECORDPTR_LENGTH);
//         }

//         // printlogf(2, "Data record: 0x%06x (%d)\n", cur_data_ptr, cur_data_ptr);
//         // printlogf(2, "Data length: 0x%04x (%d)\n", cur_data_len, cur_data_len);

//         file_len += cur_data_len;

//         // printf("Writing...\n");
//         fwrite(*buffer + cur_data_ptr, 1, cur_data_len, fp);
//         // printf("Done writing.\n");

//         // printlogf(2, "Next entry record: 0x%06x\n", next_pos);
//         if (next_pos > buffer_len && next_pos != NULL_PTR) {
//             printf("siboimg: detected pointer out of range (0x%06x)\n", next_pos);
//             exit(EXIT_FAILURE);
//         }

//         // if (file_flags & ENTRY_FLAG_NOENTRYRECORD) printlogf(2, "Last entry flag set.\n");

//         if (!((file_flags & ENTRY_FLAG_NOENTRYRECORD) == 0 && next_pos != NULL_PTR)) {
//             // printlogf(2, "End of file.\n");
//             break;
//         }
//         cur_pos = next_pos;
//     }
    
//     fclose(fp);

//     ubuf.modtime = unixtime;
//     utime(localpath, &ubuf);

//     // printlogf(1, "Total file size: %d (in %d records)\n", file_len, entry_count);
// }

void walkpath(int pos, char path[], char *buffer[], const char img_name[], const long buffer_len) {
    char entry_name[9], entry_ext[4], entry_filename[13], newpath[128];
    char entry_flags, entry_properties;
    unsigned int first_entry_ptr = 0; //, next_entry_ptr = 0;
    bool is_last_entry, is_file, is_readonly, is_hidden, is_system;
    char localpath[256];
    struct tm tm;
    time_t unixtime;
    char datetime[20];
    struct PsiDateTime psidatetime;

    // printf("VERBOSITY: %d\n", switches.verbose);

    while (true) {
        memcpy(&entry_flags, *buffer + (pos + ENTRY_FLAGS_OFFSET), ENTRY_FLAGS_LENGTH);
        memcpy(&entry_properties, *buffer + (pos + ENTRY_PROPERTIES_OFFSET), ENTRY_PROPERTIES_LENGTH);
        memcpy(entry_name, *buffer + (pos + ENTRY_NAME_OFFSET), ENTRY_NAME_LENGTH);
        entry_name[8] = 0;
        rtrim(entry_name);
        memcpy(entry_ext, *buffer + (pos + ENTRY_EXT_OFFSET), ENTRY_EXT_LENGTH);
        entry_ext[3] = 0;
        rtrim(entry_ext);

        strcpy(entry_filename, entry_name);
        if (strlen(entry_ext)) {
            strcat(entry_filename, ".");
            strcat(entry_filename, entry_ext);
        }

        is_file = (entry_flags & ENTRY_FLAG_ISFILE);
        is_last_entry = (entry_flags & ENTRY_FLAG_ISLASTENTRY);
        is_readonly = (entry_properties & ENTRY_PROPERTY_ISREADONLY);
        is_hidden = (entry_properties & ENTRY_PROPERTY_ISHIDDEN);
        is_system = (entry_properties & ENTRY_PROPERTY_ISSYSTEM);

        if (entry_flags & ENTRY_FLAG_ENTRYISVALID) {
            if (strlen(path)) {
                // printf("%s%s", path, entry_filename);
                Serial.print(path);
                Serial.print(entry_filename);
            }

            if (!is_file) {
                // printf("%s", slash);
            }

            // if (switches.verbose > 0) {
            //     printf(" (");
            //     if (!strlen(path)) {
            //         printf("root directory");
            //     } else if (is_file) {
            //         printf("file");
            //     } else {
            //         printf("directory");
            //     }

            //     printf(")");
            // }

            // printf("\n");

                // if (is_readonly) printlogf(1, "Read-only flag set.\n");
                // if (is_hidden)   printlogf(1, "Hidden flag set.\n");
                // if (is_system)   printlogf(1, "System flag set.\n");
            
            memcpy(&psidatetime, *buffer + (pos + ENTRY_TIMECODE_OFFSET), ENTRY_TIMECODE_LENGTH + ENTRY_DATECODE_LENGTH);
            tm = psidateptime(psidatetime);
            unixtime = mktime(&tm);

            strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", &tm);
            // printlogf(1, "Timestamp: %s\n", datetime);

            if (!(entry_flags & ENTRY_FLAG_NOALTRECORD)) {
                Serial.println("Has an alternative record.");
            }

            memcpy(&first_entry_ptr, *buffer + (pos + ENTRY_FIRSTENTRYRECORDPTR_OFFSET), ENTRY_FIRSTENTRYRECORDPTR_LENGTH);
            // printlogf(2, "First Entry Pointer: 0x%06x\n", first_entry_ptr);

            if(first_entry_ptr > buffer_len && first_entry_ptr != NULL_PTR) {
                // printf("%s: detected pointer out of range\n", switches.called_with);
                exit(EXIT_FAILURE);
            }

            if (is_file) {
                strcpy(localpath, img_name);
                strcat(localpath, path);
                strcat(localpath, entry_filename);
                // printlogf(2, "File to be made: %s\n", localpath);
                // getfile(pos, path, buffer, localpath, unixtime, buffer_len);
                count_files++;
            } else { // it's a directory
                if (strlen(path)) {
                    strcpy(newpath, path);
                    strcat(newpath, entry_filename);
                    strcat(newpath, slash);
                    count_dirs++;
                } else {
                    strcpy(newpath, slash);
                }
                // printlogf(1, "\n");

                strcpy(localpath, img_name);
                strcat(localpath, newpath);
                // if (fsitemexists(localpath)) {
                //     if(!direxists(localpath)) {
                //         printf("%s: %s: item exists and isn't a folder", switches.called_with, localpath);
                //         exit(EXIT_FAILURE);
                //     }
                // } else {
                //     mkdir(localpath, 0777);
                // }

                walkpath(first_entry_ptr, newpath, buffer, img_name, buffer_len);
                // if (is_readonly) {
                //     chmod(localpath, (is_file) ? 0444 : 0555);
                // }
            }
        } else {
            Serial.print("Invalid entry found, skipping ");
            Serial.print((is_file) ? "file" : "directory");
            Serial.print(": ");
            Serial.println(entry_filename);
        }
        if (is_last_entry) {
            return;
        }
        memcpy(&pos, *buffer + (pos + ENTRY_NEXTENTRYPTR_OFFSET), ENTRY_NEXTENTRYPTR_LENGTH);
        // printlogf(1, "\n");
    }
}

// int main(int argc, const char **argv) {
//     FILE *fp;
//     char i;
//     long file_len;
//     char *buffer;
//     char img_name[12], volume_id[33];
//     unsigned short img_type;
//     unsigned int img_flashcount = 0, img_rootstart = 0;
//     bool only_list, ignore_attributes, ignore_modtime;

//     // strcpy(switches.called_with, argv[0]);
//     // // switches.verbose = 0;
//     // struct argparse_option options[] = {
//     //     OPT_HELP(),
//     //     OPT_BOOLEAN('l', "list", &switches.only_list, "only list the contents of the image, don't extract files"),
//     //     OPT_BOOLEAN('n', "no-attributes", &switches.ignore_attributes, "ignore file attributes"),
//     //     OPT_BOOLEAN('m', "no-modtime", &switches.ignore_modtime, "ignore file modification times"),
//     //     OPT_INTEGER('v', "verbose", &switches.verbose, "set verbosity level"),
//     //     OPT_END(),
//     // };
//     // struct argparse argparse;
//     // argparse_init(&argparse, options, usage, 0);
//     // argparse_describe(&argparse, "\nExtracts files from Psion SIBO Flash and ROM SSD images.", "");
//     // argc = argparse_parse(&argparse, argc, argv);

//     // // printf("VERBOSITY: %d\n", switches.verbose);

//     // if (argv[0] == NULL) {
//     //     printf("%s: missing argument\n", switches.called_with);
//     //     exit(EXIT_FAILURE);
//     // } else if (argc > 1) {
//     //     printf("%s: too many arguments\n", switches.called_with);
//     //     exit(EXIT_FAILURE);
//     // }

//     // if (!fileexists(argv[0])) {
//     //     printf("%s: %s: file not found\n", switches.called_with, argv[0]);
//     //     exit(EXIT_FAILURE);
//     // }

//     // fp = fopen(argv[0], "rb");
//     // fread(&img_type, 2, 1, fp);

//     // if (img_type != FEFS_TYPE) {
//     //     printf("%s: %s: Not a Psion Flash or ROM SSD image.\n", switches.called_with, argv[0]);
//     //     fclose(fp);
//     //     exit(EXIT_FAILURE); 
//     // }

//     // Get file size
//     fseek(fp, 0, SEEK_END);
//     file_len = ftell(fp);
//     rewind(fp);

//     buffer = (char *)malloc((file_len + 1) * sizeof(char)); // Enough memory for file + \0
//     fread(buffer, file_len, 1, fp);
//     fclose(fp);
    
//     // Fetch ROM Name
//     memcpy(img_name, buffer + IMAGE_NAME_OFFSET, IMAGE_NAME_LENGTH);
//     img_name[11] = 0;
//     rtrim(img_name);
//     printf("Image name: %s\n", img_name);
//     printf("Length: %ld bytes\n", file_len);

//     // Fetch Flash Count (or identify as ROM)
//     memcpy(&img_flashcount, buffer + IMAGE_FLASHCOUNT_OFFSET, IMAGE_FLASHCOUNT_LENGTH);
//     if (img_flashcount == IMAGE_ISROM) {
//         printf("ROM image.\n");
//         memcpy(volume_id, buffer + IMAGE_ROMIDSTRING_OFFSET, 32);
//     } else {
//         printf("Flashed %d times.\n", img_flashcount);
//         memcpy(volume_id, buffer + IMAGE_FLASHIDSTRING_OFFSET, 32);
//     }
//     for (i = 0; i <=32; i++) {
//         if ((unsigned char) volume_id[i] == 0xFF) {
//             volume_id[i] = 0;
//             break;
//         }
//     }
//     volume_id[32] = 0;
//     printf("Volume ID: %s\n", volume_id);

//     memcpy(&img_rootstart, buffer + IMAGE_ROOTPTR_OFFSET, IMAGE_ROOTPTR_LENGTH);
//     // printlogf(2, "Root directory starts at: 0x%06x\n", img_rootstart);
//     printf("\n");

//     walkpath(img_rootstart, "", &buffer, img_name, file_len);

//     free(buffer);
//     printf("\nExtracted %d files in %d directories.\n", count_files, count_dirs);
//     return(0);
// }




// Grabs the title from an FEFS filesystem
void getFEFSTitle(SIBOSPConnection &sibosp) {
  char img_name[12];

  sibosp.setAddress(IMAGE_NAME_OFFSET);
  sibosp.sendControlFrame(SP_SCTL_READ_MULTI_BYTE | 0); // Tell the SSD we want to read some data

  for (int b = 0; b < IMAGE_NAME_LENGTH; b++) {
    img_name[b] = sibosp.fetchDataFrame();
  }
  img_name[11] = 0;
  rtrim(img_name);

//   printf("Image name: %s\n", img_name);
  Serial.print("Image name: ");
  Serial.println(img_name);
}