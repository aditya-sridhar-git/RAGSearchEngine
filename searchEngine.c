/*
 * Mini Search Engine in C
 * Data Structures: Linked Lists, Tries, Hash Tables
 * Features: Document indexing, word frequency, keyword search
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define ALPHABET_SIZE 26
#define HASH_SIZE 1000
#define MAX_WORD_LEN 100
#define MAX_LINE_LEN 1024

/* ==================== DATA STRUCTURES ==================== */

// Linked List Node - stores document occurrences
typedef struct DocNode {
    int doc_id;
    int frequency;
    struct DocNode *next;
} DocNode;

// Trie Node - for prefix-based searching
typedef struct TrieNode {
    struct TrieNode *children[ALPHABET_SIZE];
    int is_end;
    DocNode *doc_list;  // Documents containing this word
    int total_freq;     // Total frequency across all docs
} TrieNode;

// Hash Table Entry - for O(1) word lookup
typedef struct HashEntry {
    char *word;
    TrieNode *trie_node;  // Points to trie node for this word
    struct HashEntry *next;
} HashEntry;

// Document metadata
typedef struct Document {
    int id;
    char *filename;
    int word_count;
    struct Document *next;
} Document;

// Search Engine
typedef struct SearchEngine {
    TrieNode *trie_root;
    HashEntry *hash_table[HASH_SIZE];
    Document *documents;
    int doc_count;
} SearchEngine;

/* ==================== UTILITY FUNCTIONS ==================== */

// Hash function (djb2)
unsigned long hash_func(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash % HASH_SIZE;
}

// Convert string to lowercase and remove non-alpha chars
void normalize_word(char *word) {
    char *src = word, *dst = word;
    while (*src) {
        if (isalpha(*src))
            *dst++ = tolower(*src);
        src++;
    }
    *dst = '\0';
}

/* ==================== TRIE OPERATIONS ==================== */

TrieNode* create_trie_node() {
    TrieNode *node = (TrieNode*)calloc(1, sizeof(TrieNode));
    return node;
}

// Add document occurrence to a word's document list
void add_doc_occurrence(TrieNode *node, int doc_id) {
    DocNode *curr = node->doc_list;
    
    // Check if document already exists
    while (curr) {
        if (curr->doc_id == doc_id) {
            curr->frequency++;
            node->total_freq++;
            return;
        }
        curr = curr->next;
    }
    
    // Add new document node
    DocNode *new_node = (DocNode*)malloc(sizeof(DocNode));
    new_node->doc_id = doc_id;
    new_node->frequency = 1;
    new_node->next = node->doc_list;
    node->doc_list = new_node;
    node->total_freq++;
}

// Insert word into trie
TrieNode* trie_insert(TrieNode *root, const char *word, int doc_id) {
    TrieNode *curr = root;
    
    for (int i = 0; word[i]; i++) {
        int idx = word[i] - 'a';
        if (idx < 0 || idx >= ALPHABET_SIZE) continue;
        
        if (!curr->children[idx])
            curr->children[idx] = create_trie_node();
        curr = curr->children[idx];
    }
    
    curr->is_end = 1;
    add_doc_occurrence(curr, doc_id);
    return curr;
}

// Search word in trie
TrieNode* trie_search(TrieNode *root, const char *word) {
    TrieNode *curr = root;
    
    for (int i = 0; word[i]; i++) {
        int idx = word[i] - 'a';
        if (idx < 0 || idx >= ALPHABET_SIZE) return NULL;
        if (!curr->children[idx]) return NULL;
        curr = curr->children[idx];
    }
    
    return curr->is_end ? curr : NULL;
}

// Find all words with given prefix (for autocomplete)
void trie_prefix_helper(TrieNode *node, char *prefix, int len) {
    if (!node) return;
    
    if (node->is_end) {
        prefix[len] = '\0';
        printf("  %s (freq: %d)\n", prefix, node->total_freq);
    }
    
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->children[i]) {
            prefix[len] = 'a' + i;
            trie_prefix_helper(node->children[i], prefix, len + 1);
        }
    }
}

void trie_prefix_search(TrieNode *root, const char *prefix) {
    TrieNode *curr = root;
    char buffer[MAX_WORD_LEN];
    strcpy(buffer, prefix);
    int len = strlen(prefix);
    
    for (int i = 0; prefix[i]; i++) {
        int idx = prefix[i] - 'a';
        if (idx < 0 || idx >= ALPHABET_SIZE || !curr->children[idx]) {
            printf("No words found with prefix '%s'\n", prefix);
            return;
        }
        curr = curr->children[idx];
    }
    
    printf("Words with prefix '%s':\n", prefix);
    trie_prefix_helper(curr, buffer, len);
}

/* ==================== HASH TABLE OPERATIONS ==================== */

void hash_insert(SearchEngine *engine, const char *word, TrieNode *trie_node) {
    unsigned long idx = hash_func(word);
    
    // Check if already exists
    HashEntry *curr = engine->hash_table[idx];
    while (curr) {
        if (strcmp(curr->word, word) == 0)
            return;  // Already indexed
        curr = curr->next;
    }
    
    // Insert new entry
    HashEntry *entry = (HashEntry*)malloc(sizeof(HashEntry));
    entry->word = strdup(word);
    entry->trie_node = trie_node;
    entry->next = engine->hash_table[idx];
    engine->hash_table[idx] = entry;
}

TrieNode* hash_search(SearchEngine *engine, const char *word) {
    unsigned long idx = hash_func(word);
    HashEntry *curr = engine->hash_table[idx];
    
    while (curr) {
        if (strcmp(curr->word, word) == 0)
            return curr->trie_node;
        curr = curr->next;
    }
    return NULL;
}

/* ==================== SEARCH ENGINE OPERATIONS ==================== */

SearchEngine* create_search_engine() {
    SearchEngine *engine = (SearchEngine*)calloc(1, sizeof(SearchEngine));
    engine->trie_root = create_trie_node();
    return engine;
}

// Add document to engine
void add_document(SearchEngine *engine, const char *filename) {
    Document *doc = (Document*)malloc(sizeof(Document));
    doc->id = engine->doc_count++;
    doc->filename = strdup(filename);
    doc->word_count = 0;
    doc->next = engine->documents;
    engine->documents = doc;
}

// Get document by ID
Document* get_document(SearchEngine *engine, int doc_id) {
    Document *curr = engine->documents;
    while (curr) {
        if (curr->id == doc_id) return curr;
        curr = curr->next;
    }
    return NULL;
}

// Index a single word
void index_word(SearchEngine *engine, const char *word, int doc_id) {
    char normalized[MAX_WORD_LEN];
    strncpy(normalized, word, MAX_WORD_LEN - 1);
    normalized[MAX_WORD_LEN - 1] = '\0';
    normalize_word(normalized);
    
    if (strlen(normalized) < 2) return;  // Skip very short words
    
    TrieNode *node = trie_insert(engine->trie_root, normalized, doc_id);
    hash_insert(engine, normalized, node);
}

// Index document from file
int index_document(SearchEngine *engine, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: Cannot open file '%s'\n", filename);
        return -1;
    }
    
    add_document(engine, filename);
    int doc_id = engine->doc_count - 1;
    
    char line[MAX_LINE_LEN];
    char word[MAX_WORD_LEN];
    int word_count = 0;
    
    while (fgets(line, MAX_LINE_LEN, file)) {
        char *token = strtok(line, " \t\n\r.,;:!?\"'()[]{}");
        while (token) {
            index_word(engine, token, doc_id);
            word_count++;
            token = strtok(NULL, " \t\n\r.,;:!?\"'()[]{}");
        }
    }
    
    // Update document word count
    Document *doc = get_document(engine, doc_id);
    if (doc) doc->word_count = word_count;
    
    fclose(file);
    printf("Indexed '%s' (Doc ID: %d, Words: %d)\n", filename, doc_id, word_count);
    return doc_id;
}

// Index from string (for demo purposes)
int index_text(SearchEngine *engine, const char *name, const char *text) {
    add_document(engine, name);
    int doc_id = engine->doc_count - 1;
    
    char *text_copy = strdup(text);
    char *token = strtok(text_copy, " \t\n\r.,;:!?\"'()[]{}");
    int word_count = 0;
    
    while (token) {
        index_word(engine, token, doc_id);
        word_count++;
        token = strtok(NULL, " \t\n\r.,;:!?\"'()[]{}");
    }
    
    Document *doc = get_document(engine, doc_id);
    if (doc) doc->word_count = word_count;
    
    free(text_copy);
    printf("Indexed '%s' (Doc ID: %d, Words: %d)\n", name, doc_id, word_count);
    return doc_id;
}

/* ==================== SEARCH FUNCTIONS ==================== */

// Search for exact keyword
void search_keyword(SearchEngine *engine, const char *keyword) {
    char normalized[MAX_WORD_LEN];
    strncpy(normalized, keyword, MAX_WORD_LEN - 1);
    normalize_word(normalized);
    
    printf("\n=== Search Results for '%s' ===\n", normalized);
    
    // Use hash table for O(1) lookup
    TrieNode *node = hash_search(engine, normalized);
    
    if (!node) {
        printf("No results found.\n");
        return;
    }
    
    printf("Total occurrences: %d\n\n", node->total_freq);
    
    DocNode *doc = node->doc_list;
    while (doc) {
        Document *doc_info = get_document(engine, doc->doc_id);
        printf("Document: %s (ID: %d)\n", 
               doc_info ? doc_info->filename : "Unknown", doc->doc_id);
        printf("  Frequency: %d\n", doc->frequency);
        doc = doc->next;
    }
}

// Search with prefix matching
void search_prefix(SearchEngine *engine, const char *prefix) {
    char normalized[MAX_WORD_LEN];
    strncpy(normalized, prefix, MAX_WORD_LEN - 1);
    normalize_word(normalized);
    
    printf("\n=== Prefix Search for '%s' ===\n", normalized);
    trie_prefix_search(engine->trie_root, normalized);
}

// Multi-keyword AND search
void search_multi(SearchEngine *engine, char *keywords[], int count) {
    printf("\n=== Multi-Keyword Search ===\n");
    printf("Keywords: ");
    for (int i = 0; i < count; i++)
        printf("%s ", keywords[i]);
    printf("\n\n");
    
    // Find documents containing ALL keywords
    int *doc_scores = (int*)calloc(engine->doc_count, sizeof(int));
    int *doc_matches = (int*)calloc(engine->doc_count, sizeof(int));
    
    for (int i = 0; i < count; i++) {
        char normalized[MAX_WORD_LEN];
        strncpy(normalized, keywords[i], MAX_WORD_LEN - 1);
        normalize_word(normalized);
        
        TrieNode *node = hash_search(engine, normalized);
        if (!node) {
            printf("'%s' not found in any document.\n", normalized);
            free(doc_scores);
            free(doc_matches);
            return;
        }
        
        DocNode *doc = node->doc_list;
        while (doc) {
            doc_matches[doc->doc_id]++;
            doc_scores[doc->doc_id] += doc->frequency;
            doc = doc->next;
        }
    }
    
    // Display results (documents with all keywords)
    int found = 0;
    for (int i = 0; i < engine->doc_count; i++) {
        if (doc_matches[i] == count) {
            Document *doc_info = get_document(engine, i);
            printf("Document: %s (ID: %d, Score: %d)\n",
                   doc_info ? doc_info->filename : "Unknown", i, doc_scores[i]);
            found = 1;
        }
    }
    
    if (!found)
        printf("No documents contain all keywords.\n");
    
    free(doc_scores);
    free(doc_matches);
}

// Display word frequency statistics
void show_word_frequency(SearchEngine *engine, const char *word) {
    char normalized[MAX_WORD_LEN];
    strncpy(normalized, word, MAX_WORD_LEN - 1);
    normalize_word(normalized);
    
    TrieNode *node = hash_search(engine, normalized);
    
    printf("\n=== Word Frequency: '%s' ===\n", normalized);
    
    if (!node) {
        printf("Word not found.\n");
        return;
    }
    
    printf("Total frequency: %d\n", node->total_freq);
    printf("Document breakdown:\n");
    
    DocNode *doc = node->doc_list;
    while (doc) {
        Document *doc_info = get_document(engine, doc->doc_id);
        float tf = doc_info ? (float)doc->frequency / doc_info->word_count : 0;
        printf("  %s: %d occurrences (TF: %.4f)\n",
               doc_info ? doc_info->filename : "Unknown",
               doc->frequency, tf);
        doc = doc->next;
    }
}

// List all indexed documents
void list_documents(SearchEngine *engine) {
    printf("\n=== Indexed Documents ===\n");
    Document *doc = engine->documents;
    while (doc) {
        printf("ID: %d | File: %s | Words: %d\n",
               doc->id, doc->filename, doc->word_count);
        doc = doc->next;
    }
    printf("Total: %d documents\n", engine->doc_count);
}

/* ==================== MEMORY CLEANUP ==================== */

void free_doc_list(DocNode *node) {
    while (node) {
        DocNode *next = node->next;
        free(node);
        node = next;
    }
}

void free_trie(TrieNode *node) {
    if (!node) return;
    for (int i = 0; i < ALPHABET_SIZE; i++)
        free_trie(node->children[i]);
    free_doc_list(node->doc_list);
    free(node);
}

void free_search_engine(SearchEngine *engine) {
    // Free trie
    free_trie(engine->trie_root);
    
    // Free hash table
    for (int i = 0; i < HASH_SIZE; i++) {
        HashEntry *curr = engine->hash_table[i];
        while (curr) {
            HashEntry *next = curr->next;
            free(curr->word);
            free(curr);
            curr = next;
        }
    }
    
    // Free document list
    Document *doc = engine->documents;
    while (doc) {
        Document *next = doc->next;
        free(doc->filename);
        free(doc);
        doc = next;
    }
    
    free(engine);
}

/* ==================== MAIN / DEMO ==================== */

int main() {
    printf("╔════════════════════════════════════════╗\n");
    printf("║     Mini Search Engine Demo            ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    
    SearchEngine *engine = create_search_engine();
    
    // Demo: Index sample texts
    const char *doc1 = "The quick brown fox jumps over the lazy dog. "
                       "The fox is very quick and clever.";
    
    const char *doc2 = "Data structures are fundamental to computer science. "
                       "Linked lists, trees, and hash tables are common structures.";
    
    const char *doc3 = "The brown bear lives in the forest. "
                       "Bears are quick when hunting for food.";
    
    printf("=== Indexing Documents ===\n");
    index_text(engine, "animals.txt", doc1);
    index_text(engine, "cs_basics.txt", doc2);
    index_text(engine, "wildlife.txt", doc3);
    
    // List documents
    list_documents(engine);
    
    // Search demonstrations
    search_keyword(engine, "quick");
    search_keyword(engine, "structures");
    search_keyword(engine, "python");  // Not found
    
    // Word frequency
    show_word_frequency(engine, "the");
    show_word_frequency(engine, "fox");
    
    // Prefix search
    search_prefix(engine, "qu");
    search_prefix(engine, "str");
    
    // Multi-keyword search
    char *keywords1[] = {"quick", "brown"};
    search_multi(engine, keywords1, 2);
    
    char *keywords2[] = {"data", "structures"};
    search_multi(engine, keywords2, 2);
    
    // Interactive mode hint
    printf("\n=== Extension Ideas ===\n");
    printf("1. Add interactive CLI with menu\n");
    printf("2. Index files from directory\n");
    printf("3. Add TF-IDF ranking\n");
    printf("4. Implement phrase search\n");
    printf("5. Add Boolean operators (AND/OR/NOT)\n");
    
    // Cleanup
    free_search_engine(engine);
    printf("\nMemory cleaned up. Goodbye!\n");
    
    return 0;
}