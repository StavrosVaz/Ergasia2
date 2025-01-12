#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_PRODUCTS 20        // Ο αριθμός των προϊόντων στον κατάλογο
#define NUM_CUSTOMERS 5        // Ο αριθμός των πελατών (θυγατρικές διεργασίες)
#define NUM_ORDERS_PER_CUSTOMER 10 // Ο αριθμός παραγγελιών ανά πελάτη

// Δομή που περιγράφει ένα προϊόν στον κατάλογο
typedef struct {
    char description[50];      // Περιγραφή του προϊόντος
    float price;               // Τιμή του προϊόντος
    int item_count;            // Διαθέσιμα τεμάχια
    int request_count;         // Σύνολο αιτημάτων για το προϊόν
    int sold_count;            // Τεμάχια που πωλήθηκαν
    char unsatisfied_users[100][50]; // Λίστα μη εξυπηρετημένων χρηστών
    int unsatisfied_count;     // Αριθμός μη εξυπηρετημένων χρηστών
} Product;

Product catalog[NUM_PRODUCTS]; // Κατάλογος προϊόντων

// Αρχικοποίηση του καταλόγου προϊόντων
void initialize_catalog() {
    for (int i = 0; i < NUM_PRODUCTS; i++) {
        snprintf(catalog[i].description, sizeof(catalog[i].description), "Product_%d", i + 1);
        catalog[i].price = (float)((i + 1) * 10); // Ορισμός τιμής του προϊόντος
        catalog[i].item_count = 2;               // Αρχικά 2 διαθέσιμα τεμάχια
        catalog[i].request_count = 0;
        catalog[i].sold_count = 0;
        catalog[i].unsatisfied_count = 0;
    }
}

// Διαχείριση μιας παραγγελίας
void process_order(int product_index, int customer_id, char *response) {
    if (product_index < 0 || product_index >= NUM_PRODUCTS) {
        snprintf(response, 100, "Invalid product index\n"); // Λάθος δείκτης προϊόντος
        return;
    }

    Product *product = &catalog[product_index]; // Πρόσβαση στο προϊόν
    product->request_count++;                   // Αύξηση αιτημάτων για το προϊόν

    if (product->item_count > 0) { // Έλεγχος αν υπάρχουν διαθέσιμα τεμάχια
        product->item_count--;     // Μείωση διαθέσιμων τεμαχίων
        product->sold_count++;     // Αύξηση πωλήσεων
        snprintf(response, 100, "Order successful: %s, Price=%.2f\n", product->description, product->price);
    } else {
        snprintf(response, 100, "Order failed: %s is out of stock\n", product->description);
        snprintf(product->unsatisfied_users[product->unsatisfied_count], 50, "Customer_%d", customer_id);
        product->unsatisfied_count++; // Αύξηση μη εξυπηρετημένων χρηστών
    }
}

// Εμφάνιση συγκεντρωτικής αναφοράς
void print_summary() {
    printf("\n=== Summary Report ===\n");
    float total_revenue = 0; // Συνολικά έσοδα
    int total_requests = 0, total_success = 0, total_failures = 0;

    for (int i = 0; i < NUM_PRODUCTS; i++) {
        Product *product = &catalog[i];
        total_requests += product->request_count;    // Σύνολο αιτημάτων
        total_success += product->sold_count;       // Επιτυχημένες πωλήσεις
        total_failures += product->unsatisfied_count; // Αποτυχημένες παραγγελίες
        total_revenue += product->sold_count * product->price; // Υπολογισμός εσόδων

        // Εκτύπωση αναφοράς για κάθε προϊόν
        printf("Product: %s\n", product->description);
        printf("Requests: %d\n", product->request_count);
        printf("Sold: %d\n", product->sold_count);
        printf("Unsatisfied Users: ");
        if (product->unsatisfied_count > 0) {
            for (int j = 0; j < product->unsatisfied_count; j++) {
                printf("%s ", product->unsatisfied_users[j]);
            }
        } else {
            printf("None");
        }
        printf("\n\n");
    }

    // Εκτύπωση συνολικών στατιστικών
    printf("=== Overall Statistics ===\n");
    printf("Total Requests: %d\n", total_requests);
    printf("Successful Orders: %d\n", total_success);
    printf("Failed Orders: %d\n", total_failures);
    printf("Total Revenue: %.2f\n", total_revenue);
}

int main() {
    int pipe_client_to_server[2], pipe_server_to_client[2];

    // Δημιουργία σωλήνων επικοινωνίας (pipes)
    if (pipe(pipe_client_to_server) == -1 || pipe(pipe_server_to_client) == -1) {
        perror("Pipe failed");
        exit(EXIT_FAILURE);
    }

    initialize_catalog(); // Αρχικοποίηση καταλόγου προϊόντων

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pid_t pid = fork(); // Δημιουργία θυγατρικής διεργασίας (πελάτη)

        if (pid == 0) { // Παιδί (πελάτης)
            srand(time(NULL) ^ getpid()); // Τυχαίος αριθμός για κάθε πελάτη

            close(pipe_client_to_server[0]); // Κλείσιμο σωλήνα ανάγνωσης από τον πελάτη
            close(pipe_server_to_client[1]); // Κλείσιμο σωλήνα εγγραφής στον πελάτη

            for (int j = 0; j < NUM_ORDERS_PER_CUSTOMER; j++) {
                int product_index = rand() % NUM_PRODUCTS; // Τυχαίο προϊόν
                write(pipe_client_to_server[1], &product_index, sizeof(product_index)); // Αποστολή παραγγελίας

                char response[100];
                read(pipe_server_to_client[0], response, sizeof(response)); // Ανάγνωση απάντησης
                printf("Customer_%d received: %s", i + 1, response);

                sleep(1); // Αναμονή 1 δευτερολέπτου μεταξύ παραγγελιών
            }
            exit(0); // Τερματισμός πελάτη
        }
    }

    // Γονιός (eshop server)
    close(pipe_client_to_server[1]); // Κλείσιμο σωλήνα εγγραφής στον διακομιστή
    close(pipe_server_to_client[0]); // Κλείσιμο σωλήνα ανάγνωσης από τον διακομιστή

    // Επεξεργασία παραγγελιών
    for (int i = 0; i < NUM_CUSTOMERS * NUM_ORDERS_PER_CUSTOMER; i++) {
        int product_index;
        read(pipe_client_to_server[0], &product_index, sizeof(product_index)); // Ανάγνωση παραγγελίας

        char response[100];
        process_order(product_index, i / NUM_ORDERS_PER_CUSTOMER + 1, response); // Επεξεργασία παραγγελίας
        write(pipe_server_to_client[1], response, sizeof(response)); // Αποστολή απάντησης

        sleep(1); // Χρόνος επεξεργασίας 1 δευτερόλεπτο
    }

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        wait(NULL); // Αναμονή για την ολοκλήρωση των πελατών
    }

    print_summary(); // Εκτύπωση συγκεντρωτικής αναφοράς

    return 0;
}

