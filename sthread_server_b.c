#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <strings.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void srand48(long);
double drand48(void);

#define SPADES   0
#define HEARTS   1
#define CLUBS    2
#define DIAMONDS 3

#define DRAW     4
#define DISCARD   5
#define LAIN(pile)     (5+(pile))
#define HIDDEN(pile)  (12+(pile))

#define FAILURE 0
#define SUCCESS (!FAILURE)

#define BLACK 0
#define RED   1

#define MAXLINE     8192
#define CONNECTIONS 128

char red[]       = {0x1b,0x5b,0x33,0x31,0x6d,0};
char black[]     = {0x1b,0x5b,0x33,0x30,0x6d,0};
char neutral[]   = {0x1b,0x5b,0x33,0x39,0x6d,0};
char whitebg[]   = {0x1b,0x5b,0x33,0x47,0x6d,0};
char neutralbg[] = {0x1b,0x5b,0x33,0x49,0x6d,0};

char spades[]   = {0xE2, 0x99, 0xA0, 0};
char hearts[]   = {0xE2, 0x99, 0xA5, 0};
char clubs[]    = {0xE2, 0x99, 0xA3, 0};
char diamonds[] = {0xE2, 0x99, 0xA6, 0};
     
char *suit[] = {spades,hearts,clubs,diamonds};
char *face[] = {"O","A","2","3","4","5","6","7","8","9","T","J","Q","K"};

typedef struct _client_t {
    int id;
    int connection;
    struct sockaddr_in address;
} client_t;

typedef struct _card_t {
  int player;
  int face;
  int suit;
  struct _card_t *below;
  struct _stAck_t *stack;
} card_t;

typedef struct _deck_t {
  int player;
  card_t cards[52];
} deck_t;

typedef struct _stAck_t {
  card_t *top;
  int type;
} stAck_t;

typedef struct _arena_t {
  stAck_t *suit[4];
} arena_t;

typedef struct _solitaire_t {
  int player;
  deck_t *deck;
  stAck_t *hidden[8];
  stAck_t *lain[8];
  stAck_t *discard;
  stAck_t *draw;
} solitaire_t;

stAck_t *newStack(int type) {
  stAck_t *stack = malloc(sizeof(stack));
  stack->top = NULL;
  stack->type = type;
  return stack;
}

int isRed(card_t *c) {
  return c->suit % 2 == RED;
}

int isBlack(card_t *c) {
  return c->suit % 2 == BLACK;
}

int isAce(card_t *c) {
  return c->face == 1;
}

int isKing(card_t *c) {
  return c->face == 13;
}

int isTop(card_t *c) {
  return c->stack->top == c;
}

int pileOf(stAck_t *stack) {
  return stack->type - LAIN(1) + 1;
}

int isLain(stAck_t *stack) {
  return (LAIN(1) <= stack->type) && (stack->type <= LAIN(7));
}

int isUp(card_t *c) {
  return isLain(c->stack) || (isTop(c) && (c->stack->type == DISCARD));
}

int okOn(card_t *c1, card_t *c2) {
  return ((isRed(c1) && isBlack(c2)) || (isRed(c2) && isBlack(c1)))
          && (c1->face +1 == c2->face);
}

void putRed() {
  // printf("%s",whitebg);
  printf("%s",red);
}

void putBlack() {
  // printf("%s",whitebg);
  printf("%s",black);
}


void putBack() {
  printf("%s",neutral);
  // printf("%s",neutralbg);
}

void putColorOfSuit(int s) {
  if (s % 2 == RED) {
    putRed();
  } else {
    putBlack();
  }
}

void putSuit(int s) {
  printf("%s",suit[s]);
}

void putFace(int f) {
  printf("%s",face[f]);
}

void putCard(card_t *c) {
  if (isRed(c)) {
    putRed();
  } else {
    putBlack();
  }
  putFace(c->face);
  putSuit(c->suit);
  putBack();
}

int isEmpty(stAck_t *stack) {
  return stack->top == NULL;
}

void push(card_t *card, stAck_t *stack) {
  card->below = stack->top;
  stack->top = card;
  card->stack = stack;
}

card_t *pop(stAck_t *stack) {
  card_t *card = stack->top;
  stack->top = card->below;
  card->below = NULL;
  card->stack = NULL;
  return card;
}

card_t *top(stAck_t *stack) {
  return stack->top;
}

card_t *cardOf(char *c, solitaire_t *S) {
  int i;
  if (c[0] >= '2' && c[0] <= '9') {
    i = c[0] - '0';
  } else if (c[0] == 'T' || c[0] == 't') {
    i = 10;
  } else if (c[0] == 'J' || c[0] == 'j') {
    i = 11;
  } else if (c[0] == 'Q' || c[0] == 'q') {
    i = 12;
  } else if (c[0] == 'K' || c[0] == 'k') {
    i = 13;
  } else if (c[0] == 'A' || c[0] == 'a') {
    i = 1;
  } else {
    return NULL;
  }
  int j;
  if (c[1] == 'S' || c[1] == 's') {
    j = SPADES;
  } else if (c[1] == 'H' || c[1] == 'h') {
    j = HEARTS;
  } else if (c[1] == 'C' || c[1] == 'c') {
    j = CLUBS;
  } else if (c[1] == 'D' || c[1] == 'd') {
    j = DIAMONDS;
  } else {
    return NULL;
  }
  return &S->deck->cards[j*13+i-1];
}

deck_t *newDeck() {
  // Make a fresh deck of cards.
  deck_t *deck = malloc(sizeof(deck_t));
  int i,s;
  for (i=0, s=0; s <= 3; s++) {
    for (int f=1; f <= 13; f++, i++) {
      deck->cards[i].suit = s;
      deck->cards[i].face = f;
      deck->cards[i].below = NULL;
      deck->cards[i].stack = NULL;
    }
  }
  return deck;
}

void shuffleInto(deck_t *deck, stAck_t *draw) {
  card_t *shuffle[52];

  // Get a reference to each of the 52 cards.
  for (int i = 0; i < 52; i++) {
    shuffle[i] = &deck->cards[i];
  }

  // Perform a Knuth shuffle.
  for (int i = 0; i < 52; i++) {

    // Select the next card.
    int r = (int)((52-i)*drand48());
    if (r != 0) {
      card_t *tmp = shuffle[i];
      shuffle[i] = shuffle[i+r];
      shuffle[i+r] = tmp;
    }

    // Put it onto the draw pile.
    push(shuffle[i],draw);
  }
}

arena_t *newArena(void) {
  arena_t *A = malloc(sizeof(arena_t));
  for (int s=0; s<4; s++) {
    A->suit[s] = newStack(s);
  }
  return A;
}

// ♤♤♤♤♤♤ Necessary global variable for random number seed.
long deck_seed;

solitaire_t *newSolitaire(void) {
  solitaire_t *S = (solitaire_t *)malloc(sizeof(solitaire_t));
  S->deck = newDeck();

  // Make all the stacks.
  S->draw = newStack(DRAW);
  S->discard = newStack(DISCARD);
  for (int p=1; p<=7; p++) {
    S->lain[p] = newStack(LAIN(p));
    S->hidden[p] = newStack(HIDDEN(p));
  }

  // Build the shuffled draw pile.
  shuffleInto(S->deck,S->draw);

  // Make each of the hidden piles.
  for (int i=1; i <= 7; i++) {
    for (int j=i; j <= 7; j++) {
      push(pop(S->draw),S->hidden[j]);
    }
  }

  // Flip the top card of each pile.
  for (int i=1; i <= 7; i++) {
    push(pop(S->hidden[i]),S->lain[i]);
  }

  // Flip up the first card.
  push(pop(S->draw),S->discard);

  return S;
}

void maybeFlip(stAck_t *stack, solitaire_t *S) {
  if (isLain(stack) && isEmpty(stack)) {
    int p = pileOf(stack);
    if (!isEmpty(S->hidden[p])) {
      push(pop(S->hidden[p]),S->lain[p]);
    }
  }
}

void putStack(stAck_t *stack) {
  card_t *c = stack->top;
  while (c != NULL) {
    putCard(c);
    printf(" ");
    c = c->below;
  }
}

void putArena(arena_t *A) {
  for (int s=0; s<4; s++) {
    putColorOfSuit(s);
    putSuit(s);
    putBack();
    printf(": ");
    putStack(A->suit[s]);
    printf("\n");
  }
}

void putSolitaire(solitaire_t *S) {
  for (int p=1; p<=7; p++) {
    printf("%d: ",p);
    putStack(S->lain[p]);
    printf("[");
    putStack(S->hidden[p]);
    printf("]\n");
  }
  printf("\n[");
  putStack(S->draw);
  printf("]\n");
  putStack(S->discard);
  printf("\n");
}

int play(card_t *card, arena_t *arena, solitaire_t *S) {
  stAck_t *stack = card->stack;
  stAck_t *pile = arena->suit[card->suit];
  if (isTop(card)) {
    if (isEmpty(pile)) {
      if (isAce(card)) {
        push(pop(stack),pile);
        maybeFlip(stack,S);
        return SUCCESS;
      }
    } else {
      if (card->face == top(pile)->face+1) {
        push(pop(stack),pile);
        maybeFlip(stack,S);
        return SUCCESS;
      }
    }
  }
  return FAILURE;
}

void move(card_t *card, stAck_t *dest, solitaire_t *S) {
  stAck_t *srce = card->stack;
  card_t *top = srce->top;
  card_t *c = top;
  while (c != card) {
    c->stack = dest;
    c = c->below;
  }
  card->stack = dest;
  srce->top = card->below;
  card->below = dest->top;
  dest->top = top;
  maybeFlip(srce,S);
}

int isBottom(char *cs) {
  return strcmp(cs,"B") == 0;
}

stAck_t *freeLain(solitaire_t *S) {
  for (int p=1; p<=7; p++) {
    if (isEmpty(S->lain[p])) {
      return S->lain[p];
    }
  }
  return NULL;
}

int moveOnto(card_t *card, card_t *onto, solitaire_t *S) {

  if (onto == NULL) {

    if (isKing(card)) {
      stAck_t *dest = freeLain(S);
      if (isUp(card) && dest != NULL) {
        printf("going for it\n");
        move(card,dest,S);
        return SUCCESS;
      }
    }

  } else { 

    stAck_t *dest = onto->stack;
    if (isLain(dest) && isUp(card) && isTop(onto) && okOn(card,onto)) {
      move(card,dest,S);
      return SUCCESS;
    } 

  }

  return FAILURE;
}


// ♤♤♤♤♤♤ P-Thread Client Code
void *clientsess(void *ci) {
    client_t *client = (client_t *)ci;
    
    int id = client->id;
    int connfd = client->connection;
    
    // ♤♤♤♤♤♤ Store random number seed into global variable.
    struct timeval tp;
    gettimeofday(&tp,NULL);
    deck_seed = tp.tv_sec;
    srand48(deck_seed);
    
    // ♤♤♤♤♤♤ Send random number seed to client.
    char nseed[100];
    int z = sprintf(nseed, "%ld", deck_seed);
    write(connfd,nseed,strlen(nseed)+1);
    printf("\n Successfully sent over the random number seed %ld to client! ",deck_seed);
    printf("\n Beginning the solitaire game . . . \n\n");
    
    
    //
    // Report the client that connected.
    //
    struct hostent *hostp;
    if ((hostp = gethostbyaddr((const char *)&client->address.sin_addr.s_addr,
                               sizeof(struct in_addr),
                               AF_INET)) == NULL) {
        fprintf(stderr, "GETHOSTBYADDR failed for client %d.",id);
    }
    
    printf("♤♤♤♤♤♤ SOLITAIRE ♤♤♤♤♤♤ —— Server Side\n");
    

    
    printf("Accepted solitaire game with client %d %s (%s)\n",
           id,
           hostp->h_name,
           inet_ntoa(client->address.sin_addr));
    
    
    // ♤♤♤♤♤♤ Begin the Solitaire game.
    solitaire_t *S = newSolitaire();
    arena_t *A = newArena();
    
    char success[8] = "SUCCESS!";
    char failure[8] = "FAILURE!";
    //
    // Handle game sessions.
    //
    while (1) {
        
        putArena(A);
        putSolitaire(S);
        
        
        // Receive action from the client.
        //
        int recvlen;
        char buffer[MAXLINE];
        recvlen = read(connfd, buffer, MAXLINE);
        printf("\n ♤♤♤♤♤♤ Updated deck.");
        printf("\n");
        
        char cmd[MAXLINE];
        char c1[MAXLINE];
        char c2[MAXLINE];
        sscanf(buffer,"%s",cmd);
        
        
        if (cmd[0] == 'p') {
            // ♤♤♤♤♤♤ Playing a card
            sscanf(buffer,"%s %s",cmd,c1);
            card_t *card = cardOf(c1,S);
            
            if (play(card,A,S)) {
                write(connfd,success,strlen(success)+1);        // return SUCCESS to client
            } else {
                write(connfd,failure,strlen(failure)+1);        // return FAILURE to client
            }
            
        } else if (cmd[0] == 'm'){
            // ♤♤♤♤♤♤ Moving a card
            sscanf(buffer,"%s %s %s",cmd,c1,c2);
            
            if (moveOnto(cardOf(c1,S),cardOf(c2,S),S)) {
                write(connfd,success,strlen(success)+1);        // return SUCCESS to client
            } else {
                write(connfd,failure,strlen(failure)+1);        // return FAILURE to client
            }
            
        } else if (cmd[0] == 'n'){
            // ♤♤♤♤♤♤ Next := draw next card and put it on top
            if (!isEmpty(S->draw)) {
                push(pop(S->draw),S->discard);
                write(connfd,success,strlen(success)+1);        // return SUCCESS to client
            } else {
                write(connfd,failure,strlen(failure)+1);        // return FAILURE to client
            }
            
        } else if (cmd[0] == 'q'){
            // ♤♤♤♤♤♤ EOF. Quit functionality for 'q'.
            printf("No players left. Server is closed.\n");
            write(connfd,success,strlen(success)+1);            // return SUCCESS to client
            // ♤♤♤♤♤♤ Close the client connection
            close(connfd);
            //return NULL;
        }
    }
    
}


    
int main(int argc, char **argv)
{
    
    
    // Make sure we've been given a port to listen on.
    //
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
        
    //
    // Open a socket to listen for client connections.
    //
    int listenfd;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr,"SOCKET creation failed.\n");
        exit(-1);
    }
        
    //
    // Build the service's info into a (struct sockaddr_in).
    //
    int port = atoi(argv[1]);
    struct sockaddr_in serveraddr;
    bzero((char *) &serveraddr, sizeof(struct sockaddr_in));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
        
    //
    // Bind that socket to a port.
    //
    if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr)) < 0) {
        fprintf(stderr,"BIND failed.\n");
        exit(-1);
    }
        
    //
    // Listen for client connections on that socket.
    //
    if (listen(listenfd, CONNECTIONS) < 0) {
        fprintf(stderr,"LISTEN failed.\n");
        exit(-1);
    }
        
    fprintf(stderr,"Listening on port %d...\n",port);
    int clients = 0;
    
    //
    // Handle client sessions.
    //
    while (1) {

        //
        // Build a client's profile to send to a handler thread.
        //
        client_t *client = (client_t *)malloc(sizeof(client_t));
    
        //
        // Accept a connection from a client, get a file descriptor for communicating
        // with the client.
        //
        client->id = clients;
        socklen_t acceptlen = sizeof(struct sockaddr_in);
        if ((client->connection = accept(listenfd, (struct sockaddr *)&client->address, &acceptlen)) < 0) {
            fprintf(stderr,"ACCEPT failed.\n");
            exit(-1);
        }
        
        // ♤♤♤♤♤♤ Create a new pthread for this particular client
        pthread_t tid;
        pthread_create(&tid,NULL,clientsess,(void *)client);
        
    }
    
    close(listenfd);
    exit(0);
    
}

