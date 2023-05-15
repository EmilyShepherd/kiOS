
#ifndef _GPT_H
#define _GPT_H 1

// "EFI PART"
#define GPT_SIGNATURE 0x5452415020494645ULL

// Revision 1.0
#define GPT_VERSION 0x00010000UL

// Probably should figure out how to query the LBA size from the kernel
// but for now lets just assume it is 512
#define LBA_SIZE 512
#define LBA(x) LBA_SIZE * x

/**
 * Errors returned by scan_block_dev:
 *
 * E_NOT_GPT        A GPT header could not be read, either because the
 *                  device was too small, or a valid GPT Signature could
 *                  not be found
 * E_BAD_REVISION   The GPT header's revision did not match the expected
 *                  value, making it unsafe to continue reading
 * E_READ           There was a valid GPT Header, but the device was
 *                  smaller than expected
 * E_BAD_ENTRY_SIZE The size of each entry was too small
 * E_NOT_FOUND      A matching partition was not found after
 *                  successfully canning all entries
 */
#define E_NOT_GPT -1
#define E_BAD_REVISION -2
#define E_READ -3
#define E_BAD_ENTRY_SIZE -4
#define E_NOT_FOUND 0

/**
 * Represents a 16 byte GUID
 */
struct GUID {
  unsigned long part1;
  unsigned long part2;
};

struct GPTHeader {
  unsigned long signature;
  unsigned int revision;
  unsigned int header_size;
  unsigned int crc32_header;
  unsigned int reserved;
  unsigned long current_lba;
  unsigned long backup_lba;
  unsigned long first_lba;
  unsigned long last_lba;
  struct GUID disk_id;
  unsigned long starting_lba;
  unsigned int entries;
  unsigned int entry_size;
  unsigned int crc32_entries;
};

struct GPTEntry {
  struct GUID type;
  struct GUID id;
  unsigned long first_lba;
  unsigned long last_lba;
  unsigned long flags;
  char name[72];
};

#define MAX_ENTRY_SIZE sizeof(struct GPTEntry)
#define MIN_ENTRY_SIZE sizeof(struct GPTEntry) - sizeof(((struct GPTEntry *)0)->name)

/**
 * Determines the path of the datapart
 */
int determine_datapart(char *datapart);

#endif
