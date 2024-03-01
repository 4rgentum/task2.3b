#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Item {
  unsigned int key;
  unsigned int par;
  unsigned int offset;
  struct Item *next;
} Item;

typedef struct Table {
  int size;
  Item *first; // Просматриваемая таблица - список
  FILE *fd; // Сохраняем ещё и дескриптор файла
} Table;

char* readline();
int get_unsigned_int();
int get_int();

int load (Table *ptab, char *fname) {
  
  ptab->fd = fopen(fname, "r+b");
  if (ptab->fd == NULL) {
    return 0;
  }
  
  fread(&ptab->size, sizeof(int), 1, ptab->fd);
  int offset = sizeof(unsigned int);
  
  for (int i = 0; i < ptab->size; i++) {
    
    Item *tmp = (Item *) calloc(1, sizeof(Item));
    fread(&tmp->key, sizeof(unsigned int), 1, ptab->fd);
    fread(&tmp->par, sizeof(unsigned int), 1, ptab->fd);
    tmp->offset = offset + 2 * sizeof(unsigned int);
    tmp->next = NULL;
    offset += 3 * sizeof(unsigned int);

    tmp->next = ptab->first;
    ptab->first = tmp;
    
    fseek(ptab->fd, offset, SEEK_SET);
  }
  return 1;
}

int create(Table *ptab, char *fname) {
  
  ptab->fd = fopen(fname, "w+b");
  if (ptab->fd == NULL) {
    ptab->first = NULL;
    return 0;
  }
  fwrite(&(ptab->size), sizeof(int), 1, ptab->fd);
  return 1;
}

int save(Table *ptab) {
  
  fwrite(&ptab->size, sizeof(int), 1, ptab->fd);
  Item *tmp = ptab->first;
  int offset = tmp->offset;
  
  while (tmp) {
    fseek(ptab->fd, offset - 2 * sizeof(unsigned int), SEEK_SET);
    fwrite(&tmp->key, sizeof(unsigned int), 1, ptab->fd);
    fseek(ptab->fd, offset - sizeof(unsigned int), SEEK_SET);
    fwrite(&tmp->par, sizeof(unsigned int), 1, ptab->fd);
    offset -= 3 * sizeof(unsigned int);
    fseek(ptab->fd, offset, SEEK_SET);
    tmp = tmp->next;
  }
  
  fclose(ptab->fd);
  ptab->fd = NULL;
  
  return 1;
}

unsigned int* Search(Table *ptab, unsigned int key) {
  
  Item *tmp = ptab->first;
  Item *elem = (Item *) calloc(1, sizeof(Item));
  
  while (tmp) {
    fseek(ptab->fd, tmp->offset - 2 * sizeof(unsigned int), SEEK_SET);
    fread(&elem->key, sizeof(unsigned int), 1, ptab->fd);
    if(elem->key == key) {
      unsigned int *info = (unsigned int *) calloc(3, sizeof(unsigned int));
      info[0] = elem->key;
      fread(&elem->par, sizeof(unsigned int), 1, ptab->fd);
      info[1] = elem->par;
      fread(&info[2], sizeof(unsigned int), 1, ptab->fd);
      free(elem);
      return info;
    }
    tmp = tmp->next;
  }
  free(elem);
  return NULL;
}

int insert(Table *ptab, unsigned int key, unsigned int par, unsigned int info) {
  if (Search(ptab, key) != NULL) {
    return 1;
  }
  Item *tmp = (Item *) calloc(1, sizeof(Item));
  tmp->key = key;
  tmp->par = par;
  if (ptab->size == 0) {
    tmp->offset = 3 * sizeof(unsigned int);
    ptab->first = tmp;
  } else {
    tmp->offset = ptab->first->offset + 3 * sizeof(unsigned int);
    tmp->next = ptab->first;
    ptab->first = tmp;
  }
  fseek(ptab->fd, ptab->first->offset, SEEK_SET);
  fwrite(&info, sizeof(unsigned int), 1, ptab->fd);
  (ptab->size)++;
  return 0;
}

int erase(Table* ptab, int key) {
  if (ptab->size == 0) {
    return 2;
  }
  Item *cur = ptab->first;
  Item *prev = NULL;
  while (cur != NULL) {
    if (cur->key == key) {
      if (prev == NULL) {
        ptab->first = cur->next;
      } else {
        prev->next = cur->next;
      }
      fseek(ptab->fd, cur->offset - 2 * sizeof(unsigned int), SEEK_SET);
      unsigned int k = 0, par = 0, info = 0;
      fwrite(&k, sizeof(unsigned int), 1, ptab->fd);
      fwrite(&par, sizeof(unsigned int), 1, ptab->fd);
      fwrite(&info, sizeof(unsigned int), 1, ptab->fd);
      free(cur);
      (ptab->size)--;
      return 0;
    }
    prev = cur;
    cur = cur->next;
  }
  return 1;
}

int printTable(Table* ptab) {
  if (ptab->fd == NULL || ptab->size == 0) {
    return 1;
  }
  Item *tmp = ptab->first;
  while (tmp != NULL) {
    fseek(ptab->fd, tmp->offset, SEEK_SET);
    unsigned int info;
    fread(&info, sizeof(unsigned int), 1, ptab->fd);
    printf("Key: %u, Info: %u\n", tmp->key, info);
    tmp = tmp->next;
  }
  return 0;
}

int TaskSearch (Table* ptab, Table *res, unsigned int parentKey, char* newFileName) {
  if (ptab->size == 0) {
    return 2;
  }
  Item *cur = ptab->first;
  while (cur != NULL) {
    if (cur->par == parentKey) {
      unsigned int info;
      fseek(ptab->fd, cur->offset, SEEK_SET);
      fread(&info, sizeof(unsigned int), 1, ptab->fd);
      insert(res, cur->key, parentKey, info);
    }
    cur = cur->next;
  }
  if (res->size == 0) {
    return 1;
  } else {
    return 0;
  }
}

void eraseRecursive(Table* ptab, Item* item) {
    if (item == NULL) {
        return;
    }
    Item* cur = ptab->first;
    Item* prev = NULL;
    while (cur != NULL) {
        if (cur->par == item->key) {
            if (prev == NULL) {
                ptab->first = cur->next;
            } else {
                prev->next = cur->next;
            }
            fseek(ptab->fd, cur->offset - 2 * sizeof(unsigned int), SEEK_SET);
            unsigned int zero = 0;
            fwrite(&zero, sizeof(unsigned int), 1, ptab->fd);
            fwrite(&zero, sizeof(unsigned int), 1, ptab->fd);
            fwrite(&zero, sizeof(unsigned int), 1, ptab->fd);

            Item* next = cur->next;
            free(cur);
            cur = next;
        } else {
            prev = cur;
            cur = cur->next;
        }
    }
    eraseRecursive(ptab, item->next);
}

int TaskErase(Table* ptab, unsigned int key) {
    if (ptab->size == 0) {
        return 2;
    }
    Item* cur = ptab->first;
    Item* prev = NULL;
    while (cur != NULL && cur->key != key) {
        prev = cur;
        cur = cur->next;
    }
    if (cur == NULL) {
        return 1;
    }
    if (prev == NULL) {
        ptab->first = cur->next;
    } else {
        prev->next = cur->next;
    }
    fseek(ptab->fd, cur->offset - 2 * sizeof(unsigned int), SEEK_SET);
    unsigned int zero = 0;
    fwrite(&zero, sizeof(unsigned int), 1, ptab->fd);
    fwrite(&zero, sizeof(unsigned int), 1, ptab->fd);
    fwrite(&zero, sizeof(unsigned int), 1, ptab->fd);
    eraseRecursive(ptab, cur);
    free(cur);
    return 0;
}

void destroyTable(Table* table) {
    Item* cur = table->first;
    while (cur != NULL) {
        Item* next = cur->next;
        free(cur);
        cur = next;
    }

    fclose(table->fd);
    ///free(table);
}

int D_Load(Table *ptab) {
  char *fname = NULL;
  printf("Input File Name:\n");
  fname = readline();
  if (fname == NULL) {
    return 0;
  }
  if (load(ptab, fname) == 0) {
    create(ptab, fname);
  }
  free(fname);
  return 1;
}

int D_Search(Table *ptab){
  printf("Input Key:\n");
  unsigned int *info = NULL;
  unsigned int key = get_unsigned_int();
  if (key == -1){ 
    printf("End Of File\n");
    return 0; 
  }
  info = Search(ptab, key);
  if (info) {
    printf("Key: %u, Info: %u\n", key, *info);
  } else {
    printf("Task cann't been done\n");
  }
  return 1;
}

int D_Insert(Table* ptab) {
  int rc;
  const char* errmsgs[] = {"Ok", "Duplicate", "No Such File"};
  printf("Input Key:\n");
  unsigned int key = get_unsigned_int();
  if (key == -1){ 
    printf("End Of File\n");
    return 0; 
  }
  printf("Input Parent:\n");
  unsigned int par = get_unsigned_int();
  if (par == -1){ 
    printf("End Of File\n");
    return 0;
  }
  printf("Input Info:\n");
  unsigned int info = get_unsigned_int();
  if (info == -1){ 
    printf("End Of File\n");
    return 0;
  }
  rc = insert(ptab, key, par, info);
  printf("\n");
  printf("%s: %u\n",errmsgs[rc], key);
  return 1;
}

int D_Erase(Table* ptab) {
  int rc;
  printf("Input Key:\n");
  unsigned int key = get_unsigned_int();
  const char* errmsgs[] = {"Ok", "This Element Doesn't Exist", "Table Is Empty"};
  if (key == -1){ 
    printf("End Of File\n");
    return 0; 
  }
  rc = erase(ptab, key);
  printf("\n");
  printf("%s \n",errmsgs[rc]);
  return 1;
}

int D_Print(Table* ptab) {
  int rc;
  const char* errmsgs[] = {"Ok", "Table Is Empty"};
  rc = printTable(ptab);
  printf("\n");
  printf("%s \n",errmsgs[rc]);
  return 1;
}

int D_TaskSearch(Table* ptab) {
  int rc;
  printf("Input Parent:\n");
  unsigned int parentKey = get_unsigned_int();
  const char* errmsgs[] = {"OK", "This Element Doesn't Exist", "Table Is Empty"};
  if (parentKey == -1) {
    printf("End Of File.\n");
    return 0;
  }
  Table res = {0, NULL, NULL};
  create(&res, "tasksearch.bin");
  rc = TaskSearch(ptab, &res, parentKey, "search_result.dat");
  if (rc == 0) {
    printTable(&res);
    printf("\n");
    destroyTable(&res);
    return 1;
  }
  printf("\n%s\n", errmsgs[rc]);
  destroyTable(&res);
  return 1;
}

int D_TaskErase(Table* ptab) {
  int rc;
  printf("Input Key:\n");
  unsigned int key = get_unsigned_int();
  const char* errmsgs[] = {"Ok", "This Element Doesn't Exist", "Table Is Empty"};
  if (key == -1) { 
    printf("End Of File\n");
    return 0;
  }
  rc = TaskErase(ptab, key);
  if (rc != 1 || rc != 2) {
    printf("\n");
    printf("%s \n", errmsgs[0]);
    return 1;
  }
  printf("\n");
  printf("%s \n", errmsgs[rc]);
  return 1;
}

int dialog(const char* msgs[], int flag){
  char* errmsg = "";
  int rc;
  do {
    puts(errmsg);
    errmsg = "Invalid Input, Repeat\n";
    for (int i = 0; i < flag; i++) {
      puts(msgs[i]);
    }
    puts("Enter:\n");
    rc = get_int();
    printf("\n");
    
  } while (rc < 0 || rc >= flag);
  return rc;
}

char* readline() {
  char buffer[81] = {0};
  char *my_string = NULL;
  int length = 0;
  int element = 0;
  do {
    int flag = 0;
    element = scanf("%80[^\n]%n", buffer, &flag);
    if (element < 0 || flag == EOF) {
      if (!my_string) {
        return NULL;
      } else if (flag == EOF) {
        break;
      }
    } else if (element > 0 && flag != EOF) {
      int chunk_len = strlen(buffer);
      int str_len = length + chunk_len;
      char* temp = realloc(my_string, str_len + 1);
      if (!temp) {
        free(my_string);
        return NULL;
      }
      my_string = temp;
      memcpy(my_string + length, buffer, chunk_len);
      length = str_len;
      my_string[str_len] = '\0';
    } else {
      scanf("%*[^\n]");
      scanf("%*c");
    }
  } while (element > 0);
    if (my_string) {
      char* temp = realloc(my_string, length + 1);
      if (!temp) {
        free(my_string);
        return NULL;
      }
      my_string = temp;
      my_string[length] = '\0';
    } else {
      my_string = calloc(1, sizeof(char));
    }
  return my_string;
}

int get_unsigned_int() {
  int number;
  int flag = scanf("%u", &number);
  if (flag == -1) {
    return -1;
  } 
  while ((flag != 1 || number < 0) && flag != EOF) {
    printf("\nInvalid Input");
    printf("\nEnter Correct Unsigned Integer Number:\n");
    scanf("%*[^\n]");
    scanf("%*c");
    flag = scanf("%u", &number);
    if (flag == -1) {
      return -1;
    }
  }
  scanf("%*[^\n]");
  scanf("%*c");
  return number;
}

int get_int() {
  int number;
  int flag = scanf("%d", &number);
  if (flag == -1) {
    return 0;
  } 
  while ((flag != 1 || number < 0) && flag != EOF) {
    printf("\nInvalid Input");
    printf("\nEnter Correct Integer Number:\n");
    scanf("%*[^\n]");
    scanf("%*c");
    flag = scanf("%d", &number);
    if (flag == -1) {
      return 0;
    }
  }
  scanf("%*[^\n]");
  scanf("%*c");
  return number;
}

int main() {
  Table ptab = {0, NULL, NULL};
  if (!create(&ptab, "test.bin")) {
    printf("Failed to create table.\n");
    return 1;
  }
  const char* msgs[] = {"0. Quit", "1. Insert", "2. Based Search", "3. Based Erase", "4. Print", "5. Task Search", "6. Task Erase", "7. Load File"};
  const int NMsgs = sizeof(msgs) / sizeof(msgs[0]);
  int (*fptr[])(Table *) = {NULL, D_Insert, D_Search, D_Erase, D_Print, D_TaskSearch, D_TaskErase, D_Load};

  int rc;
  while (rc = dialog(msgs, NMsgs)) {
    if (!fptr[rc](&ptab)){
      break;
    }
  }
  save(&ptab);
  printf("End\n");
  free(ptab.first);
  return 0;
}
