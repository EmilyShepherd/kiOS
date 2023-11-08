
#include "include/gpt.h"

#include <dirent.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

// 97aac693-d920-465a-94fe-eb59fc86dfaa
const struct GUID Datapart = {0x465AD92097AAC693, 0xAADF86FC59EBFE94};

/**
 * Compares two GUIDs together to check for equality
 */
int guid_cmp(struct GUID *a, struct GUID *b) {
  return a->part1 == b->part1 && a->part2 == b->part2;
}

/**
 * Scans the given a block device for a partition entry matching the
 * given type by GUID.
 */
int scan_block_dev(FILE *file, struct GUID *guid) {
  struct GPTHeader header;

  // Skip over LBA 0, which is always blank for legacy MBR support
  if (fseek(file, LBA(1), SEEK_SET) != 0) return E_NOT_GPT;

  // Try to read the GPT header, and confirm that its signature and
  // revision are as expected.
  if (fread(&header, sizeof(struct GPTHeader), 1, file) != 1) return E_NOT_GPT;
  if (header.signature != GPT_SIGNATURE) return E_NOT_GPT;
  if (header.revision != GPT_VERSION) return E_BAD_REVISION;

  // We would normally expect each entry in the partition table to have
  // a size of 128, and that is the size of the struct that we have, so
  // we need to reject this disk if the entries are bigger than that. If
  // they are smaller, we can cope with this by eating into the label
  // field, but no further.
  if (header.entry_size > MAX_ENTRY_SIZE) return E_BAD_ENTRY_SIZE;
  if (header.entry_size < MIN_ENTRY_SIZE) return E_BAD_ENTRY_SIZE;

  // Skip the start of the entry list. This is almost always LBA 2, but
  // the GPT spec does specify the starting LBA in the header, so we
  // read that in case there is some unusualness with the disk.
  if (fseek(file, LBA(header.starting_lba), SEEK_SET)) return E_READ;

  struct GPTEntry entry;

  for (int i = 1; i <= header.entries; i++) {
    if (fread(&entry, header.entry_size, 1, file) != 1) return E_READ;
    if (guid_cmp(guid, &entry.type)) {
      return i;
    }
  }

  return E_NOT_FOUND;
}

/**
 * Finds all of the block devices on the system and scans each for a
 * partition entry matching the given type by GUID.
 */
int scan_for_part(struct GUID *guid, char *datapart) {
  DIR *dir = opendir("/sys/block");
  struct dirent *entry;

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.') continue;

    char blk[NAME_MAX + 5];
    sprintf(blk, "/dev/%s", entry->d_name);
    FILE *file = fopen(blk, "r");
    int part = scan_block_dev(file, guid);
    fclose(file);

    if (part > 0) {
      char last = blk[strlen(blk) - 1];
      if (last <= '0' && last <= '9') {
        sprintf(datapart, "%sp%d", blk, part);
      } else {
        sprintf(datapart, "%s%d", blk, part);
      }
      closedir(dir);
      return 1;
    }
  }

  closedir(dir);
  return 0;
}

/**
 * Determines the path of the datapart
 */
int determine_datapart(char *datapart) {
  char *env = getenv("datapart");
  if (!env || strcmp(env, "auto")) {
    while(!scan_for_part((struct GUID *)&Datapart, datapart)) {
      sleep(1);
    }
    return 1;
  } else if (strncmp(env, "/dev/", 5) == 0) {
    strcpy(datapart, env);
    return 1;
  }

  return 0;
}
