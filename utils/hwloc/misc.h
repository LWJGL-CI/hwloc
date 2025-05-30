/*
 * Copyright © 2009, 2024 CNRS
 * Copyright © 2009-2025 Inria.  All rights reserved.
 * Copyright © 2009-2012 Université Bordeaux
 * Copyright © 2009-2011 Cisco Systems, Inc.  All rights reserved.
 * Copyright © 2023 Université de Reims Champagne-Ardenne.  All rights reserved.
 * See COPYING in top-level directory.
 */

#ifndef HWLOC_UTILS_MISC_H
#define HWLOC_UTILS_MISC_H

#include "private/autogen/config.h"
#include "hwloc.h"
#include "private/misc.h" /* for hwloc_strncasecmp() */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#include <fcntl.h>
#include <assert.h>

extern void usage(const char *name, FILE *where);

static __hwloc_inline void
hwloc_utils_check_api_version(const char *callname)
{
  unsigned version = hwloc_get_api_version();
  if ((version >> 16) != (HWLOC_API_VERSION >> 16)) {
    fprintf(stderr,
	    "%s compiled for hwloc API 0x%x but running on library API 0x%x.\n"
	    "You may need to point LD_LIBRARY_PATH to the right hwloc library.\n"
	    "Aborting since the new ABI is not backward compatible.\n",
	    callname, (unsigned) HWLOC_API_VERSION, version);
    exit(EXIT_FAILURE);
  }
}

static __hwloc_inline void
hwloc_utils_input_format_usage(FILE *where, int addspaces)
{
  fprintf (where, "  --input <XML file>\n");
  fprintf (where, "  -i <XML file>   %*sRead topology from XML file <path>\n",
	   addspaces, " ");
#ifdef HWLOC_LINUX_SYS
  fprintf (where, "  --input <directory>\n");
  fprintf (where, "  -i <directory>  %*sRead topology from chroot containing the /proc and /sys\n",
	   addspaces, " ");
  fprintf (where, "                  %*sof another system\n",
	   addspaces, " ");
#endif
#ifdef HWLOC_HAVE_X86_CPUID
  fprintf (where, "  --input <directory>\n");
  fprintf (where, "  -i <directory>  %*sRead topology from directory containing a CPUID dump\n",
	   addspaces, " ");
#endif
  fprintf (where, "  --input \"node:2 2\"\n");
  fprintf (where, "  -i \"node:2 2\"   %*sSimulate a fake hierarchy, here with 2 NUMA nodes of 2\n",
	   addspaces, " ");
  fprintf (where, "                  %*sprocessors\n",
	   addspaces, " ");
  fprintf (where, "  --input-format <format>\n");
  fprintf (where, "  --if <format>   %*sEnforce input format among "
	   "xml, "
#ifdef HWLOC_LINUX_SYS
	   "fsroot, "
#endif
#ifdef HWLOC_HAVE_X86_CPUID
	   "cpuid, "
#endif
	   "synthetic\n",
	   addspaces, " ");
}

struct hwloc_utils_input_format_s {
  enum hwloc_utils_input_format {
    HWLOC_UTILS_INPUT_DEFAULT,
    HWLOC_UTILS_INPUT_XML,
    HWLOC_UTILS_INPUT_FSROOT,
    HWLOC_UTILS_INPUT_SYNTHETIC,
    HWLOC_UTILS_INPUT_CPUID,
    HWLOC_UTILS_INPUT_SHMEM,
    HWLOC_UTILS_INPUT_ARCHIVE
  } format;
  int oldworkdir;
};
#define HWLOC_UTILS_INPUT_FORMAT_DEFAULT (struct hwloc_utils_input_format_s) { HWLOC_UTILS_INPUT_DEFAULT, -1 }

static __hwloc_inline enum hwloc_utils_input_format
hwloc_utils_parse_input_format(const char *name, const char *callname)
{
  if (!hwloc_strncasecmp(name, "default", 3))
    return HWLOC_UTILS_INPUT_DEFAULT;
  else if (!hwloc_strncasecmp(name, "xml", 1))
    return HWLOC_UTILS_INPUT_XML;
  else if (!hwloc_strncasecmp(name, "fsroot", 1))
    return HWLOC_UTILS_INPUT_FSROOT;
  else if (!hwloc_strncasecmp(name, "shmem", 5))
    return HWLOC_UTILS_INPUT_SHMEM;
  else if (!hwloc_strncasecmp(name, "synthetic", 1))
    return HWLOC_UTILS_INPUT_SYNTHETIC;
  else if (!hwloc_strncasecmp(name, "cpuid", 1))
    return HWLOC_UTILS_INPUT_CPUID;
  else if (!hwloc_strncasecmp(name, "archive", 1))
    return HWLOC_UTILS_INPUT_ARCHIVE;

  fprintf(stderr, "input format `%s' not supported\n", name);
  usage(callname, stderr);
  exit(EXIT_FAILURE);
}

static __hwloc_inline int
hwloc_utils_lookup_input_option(char *argv[], int argc, int *consumed_opts,
				char **inputp, struct hwloc_utils_input_format_s *input_formatp,
				const char *callname)
{
  if (!strcmp (argv[0], "--input")
	       || !strcmp (argv[0], "-i")) {
    if (argc <= 1) {
      usage (callname, stderr);
      exit(EXIT_FAILURE);
    }
    if (strlen(argv[1]))
      *inputp = argv[1];
    else
      *inputp = NULL;
    *consumed_opts = 1;
    return 1;
  }
  else if (!strcmp (argv[0], "--input-format")
	   || !strcmp (argv[0], "--if")) {
    if (argc <= 1) {
      usage (callname, stderr);
      exit(EXIT_FAILURE);
    }
    *input_formatp = HWLOC_UTILS_INPUT_FORMAT_DEFAULT;
    input_formatp->format = hwloc_utils_parse_input_format (argv[1], callname);
    *consumed_opts = 1;
    return 1;
  }

  return 0;
}

static __hwloc_inline enum hwloc_utils_input_format
hwloc_utils_autodetect_input_format(const char *input, int verbose)
{
  struct stat inputst;
  int err;
  err = stat(input, &inputst);
  if (err < 0) {
    if (verbose > 0)
      printf("assuming `%s' is a synthetic topology description\n", input);
    return HWLOC_UTILS_INPUT_SYNTHETIC;
  }
  if (S_ISREG(inputst.st_mode)) {
    size_t len = strlen(input);
    if (len >= 6 && !strcmp(input+len-6, ".shmem")) {
      if (verbose > 0)
	printf("assuming `%s' is a shmem topology file\n", input);
      return HWLOC_UTILS_INPUT_SHMEM;
    }
    if ((len >= 7 && !strcmp(input+len-7, ".tar.gz"))
        || (len >= 8 && !strcmp(input+len-8, ".tar.bz2"))) {
      if (verbose > 0)
	printf("assuming `%s' is an archive topology file\n", input);
      return HWLOC_UTILS_INPUT_ARCHIVE;
    }
    if (verbose > 0)
      printf("assuming `%s' is a XML file\n", input);
    return HWLOC_UTILS_INPUT_XML;
  }
  if (S_ISDIR(inputst.st_mode)) {
    char *childpath;
    struct stat childst;
    childpath = malloc(strlen(input) + 10); /* enough for appending /sys, /proc or /pu0 */
    if (childpath) {
      snprintf(childpath, strlen(input) + 10, "%s/pu0", input);
      if (stat(childpath, &childst) == 0 && S_ISREG(childst.st_mode)) {
	if (verbose > 0)
	  printf("assuming `%s' is a cpuid dump\n", input);
	free(childpath);
	return HWLOC_UTILS_INPUT_CPUID;
      }
      snprintf(childpath, strlen(input) + 10, "%s/proc", input);
      if (stat(childpath, &childst) == 0 && S_ISDIR(childst.st_mode)) {
	if (verbose > 0)
	  printf("assuming `%s' is a file-system root\n", input);
	free(childpath);
	return HWLOC_UTILS_INPUT_FSROOT;
      }
    }
    free(childpath);
  }
  fprintf (stderr, "Unrecognized input file: %s\n", input);
  return HWLOC_UTILS_INPUT_DEFAULT;
}

static __hwloc_inline int
hwloc_utils_enable_input_format(struct hwloc_topology *topology, unsigned long flags,
				const char *input,
				struct hwloc_utils_input_format_s *input_formatp,
				int verbose, const char *callname)
{
  enum hwloc_utils_input_format *input_format = &input_formatp->format;
  if (*input_format == HWLOC_UTILS_INPUT_DEFAULT && !strcmp(input, "-.xml")) {
    *input_format = HWLOC_UTILS_INPUT_XML;
    input = "-";
  }

  if (*input_format == HWLOC_UTILS_INPUT_DEFAULT) {
    *input_format = hwloc_utils_autodetect_input_format(input, verbose);
    if (*input_format == HWLOC_UTILS_INPUT_DEFAULT) {
      usage (callname, stderr);
      return EXIT_FAILURE;
    }
  }

  switch (*input_format) {
  case HWLOC_UTILS_INPUT_XML:
    if (!strcmp(input, "-"))
      input = "/dev/stdin";
    if (hwloc_topology_set_xml(topology, input)) {
      perror("Setting source XML file");
      return EXIT_FAILURE;
    }
    break;

  case HWLOC_UTILS_INPUT_FSROOT: {
#ifdef HWLOC_LINUX_SYS
    char *env;
    if (asprintf(&env, "HWLOC_FSROOT=%s", input) < 0)
      fprintf(stderr, "Failed to pass input filesystem root directory to HWLOC_FSROOT environment variable\n");
    else
      putenv(env);
    putenv((char *) "HWLOC_DUMPED_HWDATA_DIR=/var/run/hwloc");
    env = getenv("HWLOC_COMPONENTS");
    if (env)
      fprintf(stderr, "Cannot force linux component first because HWLOC_COMPONENTS environment variable is already set to %s.\n", env);
    else
      putenv((char *) "HWLOC_COMPONENTS=linux,pci,stop");
    /* normally-set flags are overriden by envvar-forced backends */
    if (flags & HWLOC_TOPOLOGY_FLAG_IS_THISSYSTEM)
      putenv((char *) "HWLOC_THISSYSTEM=1");
#else /* HWLOC_LINUX_SYS */
    fprintf(stderr, "This installation of hwloc does not support changing the file-system root, sorry.\n");
    exit(EXIT_FAILURE);
#endif /* HWLOC_LINUX_SYS */
    break;
  }

  case HWLOC_UTILS_INPUT_CPUID: {
#ifdef HWLOC_HAVE_X86_CPUID
    size_t len = strlen("HWLOC_CPUID_PATH=")+strlen(input)+1;
    char *env = malloc(len);
    if (!env) {
      fprintf(stderr, "Failed to pass input cpuid dump path to HWLOC_CPUID_PATH environment variable\n");
    } else {
      snprintf(env, len, "HWLOC_CPUID_PATH=%s", input);
      putenv(env);
    }
    env = getenv("HWLOC_COMPONENTS");
    if (env)
      fprintf(stderr, "Cannot force x86 component first because HWLOC_COMPONENTS environment variable is already set to %s.\n", env);
    else
      putenv((char *) "HWLOC_COMPONENTS=x86,stop");
    /* normally-set flags are overriden by envvar-forced backends */
    if (flags & HWLOC_TOPOLOGY_FLAG_IS_THISSYSTEM)
      putenv((char *) "HWLOC_THISSYSTEM=1");
#else
    fprintf(stderr, "This installation of hwloc does not support loading from a cpuid dump, sorry.\n");
    exit(EXIT_FAILURE);
#endif
    break;
  }

  case HWLOC_UTILS_INPUT_ARCHIVE: {
#ifdef HWLOC_LINUX_SYS
    char mntpath[] = "/tmp/tmpdir.hwloc.archivemount.XXXXXX";
    char mntcmd[512];
    char umntcmd[512];
    DIR *dir;
    struct dirent *dirent;
    /* oldworkdir == -1 -> close would fail if !defined(O_PATH), but we don't care */
    struct hwloc_utils_input_format_s sub_input_format = HWLOC_UTILS_INPUT_FORMAT_DEFAULT;
    char *subdir = NULL;
    int err;

#ifdef O_PATH
    if (-1 == input_formatp->oldworkdir) { /* if archivemount'ed recursively, only keep the first oldworkdir */
        sub_input_format.oldworkdir = open(".", O_DIRECTORY|O_PATH); /* more efficient than getcwd(3) */
        if (sub_input_format.oldworkdir < 0) {
            perror("Saving current working directory");
            return EXIT_FAILURE;
        }
    }
#endif
    if (!mkdtemp(mntpath)) {
      perror("Creating archivemount directory");
      close(sub_input_format.oldworkdir);
      return EXIT_FAILURE;
    }
    snprintf(mntcmd, sizeof(mntcmd), "archivemount -o ro %s %s", input, mntpath);
    err = system(mntcmd);
    if (err) {
      perror("Archivemount'ing the archive");
      rmdir(mntpath);
      close(sub_input_format.oldworkdir);
      return EXIT_FAILURE;
    }
    snprintf(umntcmd, sizeof(umntcmd), "umount -l %s", mntpath);

    /* enter the mount point and stay there so that we can umount+rmdir immediately but still use it later */
    if (chdir(mntpath) < 0) {
      perror("Entering the archivemount'ed archive");
      if (system(umntcmd) < 0)
        perror("Unmounting the archivemount'ed archive (ignored)");
      rmdir(mntpath);
      close(sub_input_format.oldworkdir);
      return EXIT_FAILURE;
    }
    if (system(umntcmd) < 0)
      perror("Unmounting the archivemount'ed archive (ignored)");
    rmdir(mntpath);

    /* there should be a single subdirectory in the archive */
    dir = opendir(".");
    while ((dirent = readdir(dir)) != NULL) {
      if (strcmp(dirent->d_name, ".") && strcmp(dirent->d_name, "..")) {
        subdir = dirent->d_name;
        break;
      }
    }
    closedir(dir);

    if (!subdir) {
      perror("No subdirectory in archivemount directory");
      close(sub_input_format.oldworkdir);
      return EXIT_FAILURE;
    }

    /* call ourself recursively on subdir, it should be either a fsroot or a cpuid directory */
    err = hwloc_utils_enable_input_format(topology, flags, subdir, &sub_input_format, verbose, callname);
    if (!err)
      *input_formatp = sub_input_format;
    else {
      close(sub_input_format.oldworkdir);
      return err;
    }
#else
    fprintf(stderr, "This installation of hwloc does not support loading from an archive, sorry.\n");
    exit(EXIT_FAILURE);
#endif
    break;
  }

  case HWLOC_UTILS_INPUT_SYNTHETIC:
    if (hwloc_topology_set_synthetic(topology, input)) {
      perror("Setting synthetic topology description");
      return EXIT_FAILURE;
    }
    break;

  case HWLOC_UTILS_INPUT_SHMEM:
    break;

  case HWLOC_UTILS_INPUT_DEFAULT:
    assert(0);
  }

  return 0;
}

static __hwloc_inline void
hwloc_utils_disable_input_format(struct hwloc_utils_input_format_s *input_format)
{
  if (-1 < input_format->oldworkdir) {
#ifdef HWLOC_LINUX_SYS
#ifdef O_PATH
    int err = fchdir(input_format->oldworkdir);
    if (err) {
      perror("Restoring current working directory");
    }
#else
    fprintf(stderr, "Couldn't restore working directory. Errors may arise.\n");
#endif
    close(input_format->oldworkdir);
    input_format->oldworkdir = -1;
#else
    fprintf(stderr, "oldworkdir should not have been changed. You should not have reached this execution branch.\n");
    exit(EXIT_FAILURE);
#endif
  }
}

static __hwloc_inline void
hwloc_utils_print_distance_matrix(FILE *output, unsigned nbobjs, hwloc_obj_t *objs, hwloc_uint64_t *matrix, int logical, int show_types)
{
  unsigned i, j;
#define MATRIX_ITEM_SIZE_MAX 17 /* 16 + ending \0 */
  char *headers;
  char *values;
  char *buf;
  size_t len, max;

  headers = malloc((nbobjs+1)*MATRIX_ITEM_SIZE_MAX);
  values = malloc(nbobjs*nbobjs*MATRIX_ITEM_SIZE_MAX);
  if (!headers || !values) {
    free(headers);
    free(values);
    return;
  }

  snprintf(headers, MATRIX_ITEM_SIZE_MAX, "           index" /* 16 */);
  max = 5;
  /* prepare column headers */
  for(i=0, buf = headers + MATRIX_ITEM_SIZE_MAX;
      i<nbobjs;
      i++, buf += MATRIX_ITEM_SIZE_MAX) {
    char tmp[MATRIX_ITEM_SIZE_MAX];
    hwloc_obj_t obj = objs[i];
    unsigned index = logical ? obj->logical_index : obj->os_index;
    if (obj->type == HWLOC_OBJ_OS_DEVICE)
      len = snprintf(tmp, MATRIX_ITEM_SIZE_MAX,
                     "%s", obj->name);
    else if (obj->type == HWLOC_OBJ_PCI_DEVICE)
      len = snprintf(tmp, MATRIX_ITEM_SIZE_MAX,
                     "%04x:%02x:%02x.%01x",
                     obj->attr->pcidev.domain, obj->attr->pcidev.bus, obj->attr->pcidev.dev, obj->attr->pcidev.func);
    else if (show_types)
      len = snprintf(tmp, MATRIX_ITEM_SIZE_MAX,
                     "%s:%d", hwloc_obj_type_string(obj->type), (int) index);
    else
      len = snprintf(tmp, MATRIX_ITEM_SIZE_MAX,
                     "%d", (int) index);
    if (len >= max)
      max = len;
    /* store it at the end of the slot in headers */
    memcpy(buf + (MATRIX_ITEM_SIZE_MAX - len - 1), tmp, len+1);
    /* and pad with spaces at the begining */
    memset(buf, ' ', MATRIX_ITEM_SIZE_MAX - len - 1);
  }
  /* prepare values */
  for(i=0, buf = values;
      i<nbobjs;
      i++) {
    for(j=0; j<nbobjs; j++, buf += MATRIX_ITEM_SIZE_MAX) {
     char tmp[MATRIX_ITEM_SIZE_MAX];
     len = snprintf(tmp, MATRIX_ITEM_SIZE_MAX, "%llu", (unsigned long long) matrix[i*nbobjs+j]);
      if (len >= max)
        max = len;
      /* store it at the end of the slot in values */
      memcpy(buf + (MATRIX_ITEM_SIZE_MAX - len - 1), tmp, len+1);
      /* and pad with spaces at the begining */
      memset(buf, ' ', MATRIX_ITEM_SIZE_MAX - len - 1);
    }
  }

  /* now display everything */
  for(i=0; i<nbobjs + 1; i++)
    fprintf(output, " %s", headers + i*MATRIX_ITEM_SIZE_MAX + MATRIX_ITEM_SIZE_MAX-max-1);
  fprintf(output, "\n");
  for(i=0; i<nbobjs; i++) {
    fprintf(output, " %s", headers + (i+1)*MATRIX_ITEM_SIZE_MAX + MATRIX_ITEM_SIZE_MAX-max-1);
    for(j=0; j<nbobjs; j++)
      fprintf(output, " %s", values + (i*nbobjs+j)*MATRIX_ITEM_SIZE_MAX + MATRIX_ITEM_SIZE_MAX-max-1);
    fprintf(output, "\n");
  }

  free(headers);
  free(values);
}

static __hwloc_inline int
hwloc_pid_from_number(hwloc_pid_t *pidp, int pid_number, int set_info __hwloc_attribute_unused, int verbose __hwloc_attribute_unused)
{
  hwloc_pid_t pid;
#ifdef HWLOC_WIN_SYS
  pid = OpenProcess(set_info ? PROCESS_SET_INFORMATION : PROCESS_QUERY_INFORMATION, FALSE, pid_number);
  if (!pid) {
    if (verbose) {
      DWORD error = GetLastError();
      char *message;
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		    NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char *)&message, 0, NULL);
      fprintf(stderr, "OpenProcess %d failed %lu: %s\n", pid_number, (unsigned long) error, message);
    }
    return -1;
  }
#else
  pid = pid_number;
#endif
  *pidp = pid;
  return 0;
}

static __hwloc_inline void
hwloc_lstopo_show_summary_depth(FILE *output, size_t prefixmaxlen, hwloc_topology_t topology, int depth)
{
  hwloc_obj_type_t type = hwloc_get_depth_type(topology, depth);
  unsigned nbobjs = hwloc_get_nbobjs_by_depth(topology, depth);
  if (nbobjs) {
    size_t prefixlen;
    char _types[64];
    const char *types;

    if (depth < 0)
      prefixlen = fprintf(output, "Special depth %d:", depth);
    else
      prefixlen = fprintf(output, "%*sdepth %d:", depth, "", depth);

    if (depth < 0) {
      /* use plain type, we don't want OSdev subtype since it may differ for other objects in the level */
      types = hwloc_obj_type_string(type);
    } else {
      /* use verbose type name, those are identical for all objects on normal levels */
      hwloc_obj_type_snprintf(_types, sizeof(_types), hwloc_get_obj_by_depth(topology, depth, 0), HWLOC_OBJ_SNPRINTF_FLAG_LONG_NAMES);
      types = _types;
    }

    fprintf(output, "%*s%u %s (type #%d)\n",
	    (int)(prefixmaxlen-prefixlen), "",
	    nbobjs, types, (int) type);
  }
}

static __hwloc_inline void
hwloc_lstopo_show_summary(FILE *output, hwloc_topology_t topology)
{
  int topodepth = hwloc_topology_get_depth(topology);
  int depth;
  size_t prefixmaxlen, sdepthmaxlen;

  prefixmaxlen = topodepth-1 + strlen("depth xyz:  ");
  sdepthmaxlen = strlen("Special depth -x:  ");
  if (prefixmaxlen < sdepthmaxlen)
    prefixmaxlen = sdepthmaxlen;

  for (depth = 0; depth < topodepth; depth++)
    hwloc_lstopo_show_summary_depth(output, prefixmaxlen, topology, depth);
  /* FIXME: which order? */
  hwloc_lstopo_show_summary_depth(output, prefixmaxlen, topology, HWLOC_TYPE_DEPTH_NUMANODE);
  hwloc_lstopo_show_summary_depth(output, prefixmaxlen, topology, HWLOC_TYPE_DEPTH_MEMCACHE);
  hwloc_lstopo_show_summary_depth(output, prefixmaxlen, topology, HWLOC_TYPE_DEPTH_BRIDGE);
  hwloc_lstopo_show_summary_depth(output, prefixmaxlen, topology, HWLOC_TYPE_DEPTH_PCI_DEVICE);
  hwloc_lstopo_show_summary_depth(output, prefixmaxlen, topology, HWLOC_TYPE_DEPTH_OS_DEVICE);
  hwloc_lstopo_show_summary_depth(output, prefixmaxlen, topology, HWLOC_TYPE_DEPTH_MISC);
}


/*************************
 * Importing/exporting userdata buffers without understanding/decoding/modifying them
 * Caller must putenv("HWLOC_XML_USERDATA_NOT_DECODED=1") before loading the topology.
 */

struct hwloc_utils_userdata {
  char *name;
  size_t length;
  char *buffer; /* NULL if userdata entry in the list is not meant to be exported to XML (added by somebody else) */
  struct hwloc_utils_userdata *next;
};

static __hwloc_inline void
hwloc_utils_userdata_import_cb(hwloc_topology_t topology __hwloc_attribute_unused, hwloc_obj_t obj, const char *name, const void *buffer, size_t length)
{
  struct hwloc_utils_userdata *u, **up = (struct hwloc_utils_userdata **) &obj->userdata;
  while (*up)
    up = &((*up)->next);
  *up = u = malloc(sizeof(struct hwloc_utils_userdata));
  u->name = strdup(name);
  u->length = length;
  u->buffer = strdup(buffer);
  u->next = NULL;
}

static __hwloc_inline void
hwloc_utils_userdata_export_cb(void *reserved, hwloc_topology_t topology, hwloc_obj_t obj)
{
  struct hwloc_utils_userdata *u = obj->userdata;
  while (u) {
    if (u->buffer) /* not meant to be exported to XML (added by somebody else) */
      hwloc_export_obj_userdata(reserved, topology, obj, u->name, u->buffer, u->length);
    u = u->next;
  }
}

/* to be called when importing from shmem with non-NULL userdata pointing to stuff in the other process */
static __hwloc_inline void
hwloc_utils_userdata_clear_recursive(hwloc_obj_t obj)
{
  hwloc_obj_t child;
  obj->userdata= NULL;
  for_each_child(child, obj)
    hwloc_utils_userdata_clear_recursive(child);
  for_each_memory_child(child, obj)
    hwloc_utils_userdata_clear_recursive(child);
  for_each_io_child(child, obj)
    hwloc_utils_userdata_clear_recursive(child);
  for_each_misc_child(child, obj)
    hwloc_utils_userdata_clear_recursive(child);
}

/* must be called once the caller has removed its own userdata */
static __hwloc_inline void
hwloc_utils_userdata_free(hwloc_obj_t obj)
{
  struct hwloc_utils_userdata *u = obj->userdata, *next;
  while (u) {
    next = u->next;
    assert(u->buffer);
    free(u->name);
    free(u->buffer);
    free(u);
    u = next;
  }
  obj->userdata = NULL;
}

/* must be called once the caller has removed its own userdata */
static __hwloc_inline void
hwloc_utils_userdata_free_recursive(hwloc_obj_t obj)
{
  hwloc_obj_t child;
  hwloc_utils_userdata_free(obj);
  for_each_child(child, obj)
    hwloc_utils_userdata_free_recursive(child);
  for_each_memory_child(child, obj)
    hwloc_utils_userdata_free_recursive(child);
  for_each_io_child(child, obj)
    hwloc_utils_userdata_free_recursive(child);
  for_each_misc_child(child, obj)
    hwloc_utils_userdata_free_recursive(child);
}

struct hwloc_utils_parsing_flag
{
    unsigned long ulong_flag;
    const char *str_flag;
};

#define HWLOC_UTILS_PARSING_FLAG(flag){ flag, #flag }

static __hwloc_inline void
hwloc_utils_parsing_flag_error(const char *err_message, struct hwloc_utils_parsing_flag possible_flags[], int len_possible_flags) {
  int i;
  fprintf(stderr, "Supported %s flags are substrings of:\n", err_message);
  for(i = 0; i < len_possible_flags; i++) {
    fprintf(stderr, "  ");
    fprintf(stderr, "%s", possible_flags[i].str_flag);
    fprintf(stderr, "\n");
  }
}

static __hwloc_inline unsigned long
hwloc_utils_parse_flags(char * str, struct hwloc_utils_parsing_flag possible_flags[], int len_possible_flags, const char * kind) {
  char *ptr;
  char *end;
  int ul_flag;
  int i;
  size_t j;
  unsigned long ul_flags = 0;

  ul_flag = strtoul(str, &end, 0);
  if(end != str && *end == '\0')
    return ul_flag;

  for(j=0; str[j]; j++)
    str[j] = toupper(str[j]);

  if(strcmp(str, "NONE") == 0)
    return 0;

  ptr = str;
  while (ptr) {
    int count = 0;
    unsigned long prv_flags = ul_flags;
    char *pch;
    int nosuffix = 0;

    /* skip separators at the beginning */
    ptr += strspn(ptr, ",|+");

    /* find separator after next token */
    j = strcspn(ptr, " ,|+");
    if (!j)
      break;

    if (ptr[j]) {
      /* mark the end of the token */
      ptr[j] = '\0';
      /* mark beginning of next token */
      end = ptr + j + 1;
    } else {
      /* no next token */
      end = NULL;
    }

    /* '$' means matching the end of a flag */
    pch = strchr(ptr, '$');
    if(pch) {
      nosuffix = 1;
      *pch = '\0';
    }

    for(i = 0; i < len_possible_flags; i++) {
      if(nosuffix == 1) {
        /* match the end */
        if(strcmp(ptr, possible_flags[i].str_flag + strlen(possible_flags[i].str_flag) - strlen(ptr)))
          continue;
      } else {
        /* match anywhere */
        if(!strstr(possible_flags[i].str_flag, ptr))
          continue;
      }

      if(count){
        fprintf(stderr, "Duplicate match for %s flag `%s'.\n", kind, ptr);
        hwloc_utils_parsing_flag_error(kind, possible_flags, len_possible_flags);
        return (unsigned long) - 1;
      }

      ul_flags |= possible_flags[i].ulong_flag;
      count++;
    }

    if(prv_flags == ul_flags) {
      fprintf(stderr, "Failed to parse %s flag `%s'.\n", kind, ptr);
      hwloc_utils_parsing_flag_error(kind, possible_flags, len_possible_flags);
      return (unsigned long) - 1;
    }

    ptr = end;
  }

  return ul_flags;
}

static __hwloc_inline hwloc_memattr_id_t
hwloc_utils_parse_memattr_name(hwloc_topology_t topo, const char *str)
{
  const char *name;
  hwloc_memattr_id_t id;
  int err;
  /* try by name, case insensitive */
  for(id=0; ; id++) {
    err = hwloc_memattr_get_name(topo, id, &name);
    if (err < 0)
      break;
    if (!strcasecmp(name, str))
      return id;
  }
  /* try by id */
  if (*str < '0' || *str > '9')
    return (hwloc_memattr_id_t) -1;
  id = atoi(str);
  err = hwloc_memattr_get_name(topo, id, &name);
  if (err < 0)
    return (hwloc_memattr_id_t) -1;
  else
    return id;
}

#define HWLOC_UTILS_BEST_NODE_FLAG_DEFAULT (1UL<<0) /* report all nodes if no best found */
#define HWLOC_UTILS_BEST_NODE_FLAG_STRICT (1UL<<1) /* report only best with same initiator */

static __hwloc_inline unsigned long
hwloc_utils_parse_best_node_flags(char *str)
{
  unsigned long best_memattr_flags = 0;
  char *tmp;

  tmp = strstr(str, ",default");
  if (tmp) {
    memmove(tmp, tmp+8, strlen(tmp+8)+1);
    best_memattr_flags |= HWLOC_UTILS_BEST_NODE_FLAG_DEFAULT;
  }

  tmp = strstr(str, ",strict");
  if (tmp) {
    memmove(tmp, tmp+7, strlen(tmp+7)+1);
    best_memattr_flags |= HWLOC_UTILS_BEST_NODE_FLAG_STRICT;
  }

  return best_memattr_flags;
}

static __hwloc_inline void
hwloc_utils__update_best_node(hwloc_obj_t newnode, uint64_t newvalue,
                              uint64_t *bestvalue, hwloc_bitmap_t bestnodeset,
                              unsigned long mflags)
{
  if (hwloc_bitmap_iszero(bestnodeset)) {
    /* first */
    *bestvalue = newvalue;
    hwloc_bitmap_only(bestnodeset, newnode->os_index);

  } else if (mflags & HWLOC_MEMATTR_FLAG_HIGHER_FIRST) {
    if (newvalue > *bestvalue) {
      /* higher */
      *bestvalue = newvalue;
      hwloc_bitmap_only(bestnodeset, newnode->os_index);
    } else if (newvalue == *bestvalue) {
      /* as high */
      hwloc_bitmap_set(bestnodeset, newnode->os_index);
    }

  } else {
    assert(mflags & HWLOC_MEMATTR_FLAG_LOWER_FIRST);
    if (newvalue < *bestvalue) {
      /* lower */
      *bestvalue = newvalue;
      hwloc_bitmap_only(bestnodeset, newnode->os_index);
    } else if (newvalue == *bestvalue) {
      /* as low */
      hwloc_bitmap_set(bestnodeset, newnode->os_index);
    }
  }
}

/* fill best_nodeset with best nodes.
 * if STRICT flag, only the really local ones are returned.
 * if none is best (they don't have values), return empty.
 * if none is best and DEFAULT flag, return all nodes.
 * on error, return empty.
 */
static __hwloc_inline int
hwloc_utils_get_best_node_in_array_by_memattr(hwloc_topology_t topology, hwloc_memattr_id_t id,
                                              unsigned nbnodes, hwloc_obj_t *nodes,
                                              struct hwloc_location *initiator,
                                              unsigned long flags,
                                              hwloc_nodeset_t best_nodeset,
                                              int verbose)
{
  unsigned i, j, nb = 0;
  hwloc_uint64_t *values, bestvalue = 0;
  unsigned long mflags;
  int err;

  hwloc_bitmap_zero(best_nodeset);

  err = hwloc_memattr_get_flags(topology, id, &mflags);
  if (err < 0)
    goto out;

  if (mflags & HWLOC_MEMATTR_FLAG_NEED_INITIATOR) {
    /* iterate over targets, and then on their initiators */
    for(i=0, nb = 0; i<nbnodes; i++) {
      unsigned nbi;
      struct hwloc_location *initiators;

      nbi = 0;
      err = hwloc_memattr_get_initiators(topology, id, nodes[i], 0, &nbi, NULL, NULL);
      if (err < 0) {
        hwloc_bitmap_zero(best_nodeset);
        goto none;
      }

      initiators = malloc(nbi * sizeof(*initiators));
      values = malloc(nbi * sizeof(*values));
      if (!initiators || !values) {
        hwloc_bitmap_zero(best_nodeset);
        free(initiators);
        free(values);
        goto none;
      }

      err = hwloc_memattr_get_initiators(topology, id, nodes[i], 0, &nbi, initiators, values);
      if (err < 0) {
        hwloc_bitmap_zero(best_nodeset);
        free(initiators);
        free(values);
        goto none;
      }

      for(j=0; j<nbi; j++) {
        /* does this initiator match the user location? */
        if (initiator->type != initiators[j].type)
          continue;
        switch (initiator->type) {
        case HWLOC_LOCATION_TYPE_OBJECT:
          if (initiator->location.object->type != initiators[j].location.object->type
              || initiator->location.object->gp_index != initiators[j].location.object->gp_index)
            continue;
          break;
        case HWLOC_LOCATION_TYPE_CPUSET:
          if (flags & HWLOC_UTILS_BEST_NODE_FLAG_STRICT) {
            if (!hwloc_bitmap_isincluded(initiator->location.cpuset, initiators[j].location.cpuset))
              continue;
          } else {
            if (!hwloc_bitmap_intersects(initiator->location.cpuset, initiators[j].location.cpuset))
              continue;
          }
          break;
        default:
          abort();
        }

        hwloc_utils__update_best_node(nodes[i], values[j],
                                      &bestvalue, best_nodeset,
                                      mflags);
        nb++;
        break; /* there shouldn't be multiple initiators */
      }

      free(initiators);
      free(values);
    }

  } else {
    /* no initiator, just iterate over targets */
    for(i=0; i<nbnodes; i++) {
      uint64_t value;
      if (!hwloc_memattr_get_value(topology, id, nodes[i], NULL, 0, &value)) {
        hwloc_utils__update_best_node(nodes[i], value,
                                      &bestvalue, best_nodeset,
                                      mflags);
        nb++;
      }
    }
  }

  if (hwloc_bitmap_iszero(best_nodeset)) {
  none:
    if (flags & HWLOC_UTILS_BEST_NODE_FLAG_DEFAULT) {
      if (!nb) {
        if (verbose > 0)
          printf("found no node with attribute values for this initiator, falling back to default\n");
      } else {
        fprintf(stderr, "couldn't find attribute values for all nodes, falling back to default\n");
      }

      /* try to get default nodes only */
      hwloc_bitmap_t default_nodeset = hwloc_bitmap_alloc();
      if (default_nodeset) {
        err = hwloc_topology_get_default_nodeset(topology, default_nodeset, 0);
        if (!err) {
          hwloc_bitmap_zero(best_nodeset);
          for(i=0; i<nbnodes; i++)
            if (hwloc_bitmap_isset(default_nodeset, nodes[i]->os_index))
              hwloc_bitmap_set(best_nodeset, nodes[i]->os_index);
        }
      }
      free(default_nodeset);
      /* if still empty, use all local nodes */
      if (hwloc_bitmap_iszero(best_nodeset))
        for(i=0; i<nbnodes; i++)
          hwloc_bitmap_set(best_nodeset, nodes[i]->os_index);

    } else {
      if (!nb) {
        if (verbose > 0)
          printf("found no node with attribute values for this initiator\n");
      } else {
        fprintf(stderr, "couldn't find attribute values for all nodes, returning none\n");
      }
    }
  }
  return 0;

 out:
  return -1;
}

enum hwloc_utils_cpuset_format_e {
  HWLOC_UTILS_CPUSET_FORMAT_UNKNOWN,
  HWLOC_UTILS_CPUSET_FORMAT_HWLOC,
  HWLOC_UTILS_CPUSET_FORMAT_LIST,
  HWLOC_UTILS_CPUSET_FORMAT_SYSTEMD,
  HWLOC_UTILS_CPUSET_FORMAT_TASKSET
};

static __hwloc_inline enum hwloc_utils_cpuset_format_e
hwloc_utils_parse_cpuset_format(const char *string)
{
  if (!strcmp(string, "hwloc"))
    return HWLOC_UTILS_CPUSET_FORMAT_HWLOC;
  else if (!strcmp(string, "list"))
    return HWLOC_UTILS_CPUSET_FORMAT_LIST;
  else if (!strcmp(string, "systemd-dbus-api"))
    return HWLOC_UTILS_CPUSET_FORMAT_SYSTEMD;
  else if (!strcmp(string, "taskset"))
    return HWLOC_UTILS_CPUSET_FORMAT_TASKSET;
  else
    return HWLOC_UTILS_CPUSET_FORMAT_UNKNOWN;
}

/* The AllowedCPUs and AllowedMemoryNodes systemd DBus API syntax expects a string as follows:
 * "ay 0xNNNN 0xAA [0xBB [...]]"
 * where:
 *   "0xNNNN" is the size of the array, max size 2^26.
 *   "0xAA [0xBB [...]]" is the cpuset or nodeset mask given bytes after bytes, little endian order.
 * e.g. the output of `hwloc-calc --cpuset-input-format list 0,31-32,63-64,77 --cpuset-output-format systemd-dbus-api` is
 *   "ay 0x000a 0x01 0x00 0x00 x80 0x01 0x00 0x00 0x80 0x01 0x20
 * e.g. the output of `hwloc-calc node:0 node:1 --nodeset-output-format systemd-dbus-api` is
 *   "ay 0x0001 0x03"
 * */
#define HWLOC_SYSTEMD_DBUS_API_PREFIX_FORMAT "ay 0x%04x"
#define HWLOC_SYSTEMD_DBUS_API_PREFIX_SIZE 9
#define HWLOC_SYSTEMD_DBUS_API_BYTES_FORMAT " 0x%02x"
#define HWLOC_SYSTEMD_DBUS_API_BYTES_SIZE 5
static __hwloc_inline int hwloc_utils_systemd_asprintf(char ** strp, const struct hwloc_bitmap_s * __hwloc_restrict set)
{
  int last = hwloc_bitmap_last(set);
  if (last == -1 ) {
    fprintf(stderr, "Empty and infinite sets are not supported with the systemd-dbus-api output format\n");
    exit(EXIT_FAILURE);
  }
  int bytes = last / 8 + 1; /* how many bytes to represent the bit mask */
  int buflen = HWLOC_SYSTEMD_DBUS_API_PREFIX_SIZE + HWLOC_SYSTEMD_DBUS_API_BYTES_SIZE * bytes + 1;
  int ret = 0;
  char *buf;
  buf = malloc(buflen);
  *strp = buf;
  ret = snprintf(buf, buflen, HWLOC_SYSTEMD_DBUS_API_PREFIX_FORMAT, bytes);
  unsigned long ith = 0;
  for (int byte=0; byte < bytes; byte++) {
    if (byte % HWLOC_SIZEOF_UNSIGNED_LONG == 0) {
      ith = hwloc_bitmap_to_ith_ulong(set, byte / HWLOC_SIZEOF_UNSIGNED_LONG);
    }
    ret += snprintf(buf + ret, HWLOC_SYSTEMD_DBUS_API_BYTES_SIZE + 1, HWLOC_SYSTEMD_DBUS_API_BYTES_FORMAT, (unsigned char) (ith & 0xFF));
    ith >>= 8;
  }
  assert(ith == 0); /* whenever some bytes of the ulong ith may not be consumed, they must be 0 */
  return ret;
}

static __hwloc_inline int
hwloc_utils_cpuset_format_asprintf(char **string, hwloc_const_bitmap_t set,
                                   enum hwloc_utils_cpuset_format_e cpuset_output_format)
{
  switch (cpuset_output_format) {
  case HWLOC_UTILS_CPUSET_FORMAT_HWLOC: return hwloc_bitmap_asprintf(string, set);
  case HWLOC_UTILS_CPUSET_FORMAT_LIST: return hwloc_bitmap_list_asprintf(string, set);
  case HWLOC_UTILS_CPUSET_FORMAT_SYSTEMD: return hwloc_utils_systemd_asprintf(string, set);
  case HWLOC_UTILS_CPUSET_FORMAT_TASKSET: return hwloc_bitmap_taskset_asprintf(string, set);
  default: abort();
  }
}

static __hwloc_inline int
hwloc_utils_cpuset_format_sscanf(hwloc_bitmap_t set, const char *string, enum hwloc_utils_cpuset_format_e cpuset_format)
{
  if (cpuset_format == HWLOC_UTILS_CPUSET_FORMAT_UNKNOWN) {
    /* If user doesn't enforce a format, try to guess.
     * There is some ambiguity if a list of singleton like 1,3,5 is given,
     * it may be parsed as 0x1,0x3,0x5 (hwloc) or 1-1,3-3,5-5 (list).
     * Assume list first, then hwloc, then taskset.
     */
    if (hwloc_strncasecmp(string, "0x", 2) && strchr(string, '-'))
      cpuset_format = HWLOC_UTILS_CPUSET_FORMAT_LIST;
    else if (strchr(string, ','))
      cpuset_format = HWLOC_UTILS_CPUSET_FORMAT_HWLOC;
    else
      cpuset_format = HWLOC_UTILS_CPUSET_FORMAT_TASKSET;
  }

  switch (cpuset_format) {
  case HWLOC_UTILS_CPUSET_FORMAT_HWLOC:
    return hwloc_bitmap_sscanf(set, string);
    break;
  case HWLOC_UTILS_CPUSET_FORMAT_LIST:
    return hwloc_bitmap_list_sscanf(set, string);
    break;
  case HWLOC_UTILS_CPUSET_FORMAT_TASKSET:
    return hwloc_bitmap_taskset_sscanf(set, string);
    break;
  default:
    /* HWLOC_UTILS_CPUSET_FORMAT_SYSTEMD input not supported */
    abort();
  }
}

static __hwloc_inline unsigned long
hwloc_utils_parse_restrict_flags(char * str){
  struct hwloc_utils_parsing_flag possible_flags[] = {
    HWLOC_UTILS_PARSING_FLAG(HWLOC_RESTRICT_FLAG_REMOVE_CPULESS),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_RESTRICT_FLAG_BYNODESET),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_RESTRICT_FLAG_REMOVE_MEMLESS),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_RESTRICT_FLAG_ADAPT_MISC),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_RESTRICT_FLAG_ADAPT_IO)
  };

  return hwloc_utils_parse_flags(str, possible_flags, (int) sizeof(possible_flags) / sizeof(possible_flags[0]), "restrict");
}

static __hwloc_inline unsigned long
hwloc_utils_parse_topology_flags(char * str) {
  struct hwloc_utils_parsing_flag possible_flags[] = {
    HWLOC_UTILS_PARSING_FLAG(HWLOC_TOPOLOGY_FLAG_INCLUDE_DISALLOWED),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_TOPOLOGY_FLAG_IS_THISSYSTEM),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_TOPOLOGY_FLAG_THISSYSTEM_ALLOWED_RESOURCES),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_TOPOLOGY_FLAG_IMPORT_SUPPORT),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_TOPOLOGY_FLAG_RESTRICT_TO_CPUBINDING),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_TOPOLOGY_FLAG_RESTRICT_TO_MEMBINDING),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_TOPOLOGY_FLAG_DONT_CHANGE_BINDING),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_TOPOLOGY_FLAG_NO_DISTANCES),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_TOPOLOGY_FLAG_NO_MEMATTRS),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_TOPOLOGY_FLAG_NO_CPUKINDS)
  };

  return hwloc_utils_parse_flags(str, possible_flags, (int) sizeof(possible_flags) / sizeof(possible_flags[0]), "topology");
}

static __hwloc_inline unsigned long
hwloc_utils_parse_allow_flags(char * str) {
  struct hwloc_utils_parsing_flag possible_flags[] = {
    HWLOC_UTILS_PARSING_FLAG(HWLOC_ALLOW_FLAG_ALL),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_ALLOW_FLAG_LOCAL_RESTRICTIONS),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_ALLOW_FLAG_CUSTOM)
  };

  return hwloc_utils_parse_flags(str, possible_flags, (int) sizeof(possible_flags) / sizeof(possible_flags[0]), "allow");
}

static __hwloc_inline unsigned long
hwloc_utils_parse_export_synthetic_flags(char * str) {
  struct hwloc_utils_parsing_flag possible_flags[] = {
    HWLOC_UTILS_PARSING_FLAG(HWLOC_TOPOLOGY_EXPORT_SYNTHETIC_FLAG_NO_EXTENDED_TYPES),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_TOPOLOGY_EXPORT_SYNTHETIC_FLAG_NO_ATTRS),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_TOPOLOGY_EXPORT_SYNTHETIC_FLAG_V1),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_TOPOLOGY_EXPORT_SYNTHETIC_FLAG_IGNORE_MEMORY)
  };

  return hwloc_utils_parse_flags(str, possible_flags, (int) sizeof(possible_flags) / sizeof(possible_flags[0]), "synthetic");
}

static __hwloc_inline unsigned long
hwloc_utils_parse_export_xml_flags(char * str) {
  struct hwloc_utils_parsing_flag possible_flags[] = {
    HWLOC_UTILS_PARSING_FLAG(HWLOC_TOPOLOGY_EXPORT_XML_FLAG_V2)
  };

  return hwloc_utils_parse_flags(str, possible_flags, (int) sizeof(possible_flags) / sizeof(possible_flags[0]), "xml");
}

static __hwloc_inline unsigned long
hwloc_utils_parse_distances_add_flags(char * str) {
  struct hwloc_utils_parsing_flag possible_flags[] = {
    HWLOC_UTILS_PARSING_FLAG(HWLOC_DISTANCES_ADD_FLAG_GROUP),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_DISTANCES_ADD_FLAG_GROUP_INACCURATE)
  };

  return hwloc_utils_parse_flags(str, possible_flags, (int) sizeof(possible_flags) / sizeof(possible_flags[0]), "distances_add");
}

static __hwloc_inline unsigned long
hwloc_utils_parse_memattr_flags(char *str) {
  struct hwloc_utils_parsing_flag possible_flags[] = {
    HWLOC_UTILS_PARSING_FLAG(HWLOC_MEMATTR_FLAG_HIGHER_FIRST),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_MEMATTR_FLAG_LOWER_FIRST),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_MEMATTR_FLAG_NEED_INITIATOR)
  };

  return hwloc_utils_parse_flags(str, possible_flags, (int) sizeof(possible_flags) / sizeof(possible_flags[0]), "memattr");
}

static __hwloc_inline unsigned long
hwloc_utils_parse_local_numanode_flags(char *str) {
  struct hwloc_utils_parsing_flag possible_flags[] = {
    HWLOC_UTILS_PARSING_FLAG(HWLOC_LOCAL_NUMANODE_FLAG_LARGER_LOCALITY),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_LOCAL_NUMANODE_FLAG_SMALLER_LOCALITY),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_LOCAL_NUMANODE_FLAG_INTERSECT_LOCALITY),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_LOCAL_NUMANODE_FLAG_ALL)
  };

  return hwloc_utils_parse_flags(str, possible_flags, (int) sizeof(possible_flags) / sizeof(possible_flags[0]), "local_numanode");
}

static __hwloc_inline unsigned long
hwloc_utils_parse_obj_snprintf_flags(char *str) {
  struct hwloc_utils_parsing_flag possible_flags[] = {
    HWLOC_UTILS_PARSING_FLAG(HWLOC_OBJ_SNPRINTF_FLAG_OLD_VERBOSE),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_OBJ_SNPRINTF_FLAG_LONG_NAMES),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_OBJ_SNPRINTF_FLAG_SHORT_NAMES),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_OBJ_SNPRINTF_FLAG_MORE_ATTRS),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_OBJ_SNPRINTF_FLAG_NO_UNITS),
    HWLOC_UTILS_PARSING_FLAG(HWLOC_OBJ_SNPRINTF_FLAG_UNITS_1000)
  };
  return hwloc_utils_parse_flags(str, possible_flags, (int) sizeof(possible_flags) / sizeof(possible_flags[0]), "obj_snprintf");
}

#endif /* HWLOC_UTILS_MISC_H */
