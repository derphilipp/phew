#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <string.h>
#define SERVERVER "0.1"	     		/* Die Versionsnummer */
#define SERVERNAME "Phew"    		/* Der Name des Webservers */
#define PORT 80	     		/* Der zu öffnende Port - HTTP ist 
Standartmäßig  80 (nur root darf niedrige Ports öffnen !*/	
#define BUFFERSIZE 1024  	/* Die größe des Dateipuffers */ 
#define STDFILE "index.htm" 	/* Name der Datei, die geöffnet wird, wenn keine Angaben gemacht wurden*/                         
#define STDDIR "/home/ppweissm/webserver/html"
#define MAXCTYPELENGHT 30 /*Die maximale länge eines Content-Types*/

/*Socket Adressen Struktur*/
#if 0
struct socketaddr_in
{
	short int sin_family; /* AF_INET*/
	unsigned short sin_port; /* Der zu verwendende Port*/
	struct in_addr sin_addr; /* Die zu verwendende IP Adresse*/ 
};
#endif


pid_t pid; /* Die ID des laufenden Prozesses */


/* ---------------------------------------------------------------------------------------- */
/* Überprüft die Anfrage, ob sie eine HEAD, eine GET oder eine ungültige Anfrage ist        */
/* und gibt 1 bei einer GET, 2 bei einer HEAD und -1 bei einer ungueltigen Anfrage zurueck  */
/* ---------------------------------------------------------------------------------------- */
int request_type (char y[BUFFERSIZE])
{

	if(			y[0]=='G'
			&&	y[1]=='E'
			&&	y[2]=='T'
			&&	y[3]==' '
	  )
		return(1);       /* Dann war es GET */
	else
	{
		if (			y[0]=='H'
				&&	y[1]=='E'
				&&	y[2]=='A'
				&&	y[3]=='D'	
				&&	y[4]==' '
		   )
			return(2); /* Dann war es HEAD */
		else
			return(-1); /* Ungültige Anfrage */
	}
}




/* ----------------------------------------------------------------------------------------------------------------------*/
/* Ueberprueft ob ein Datei vorhanden ist, oder ob es sich um ein Verzeichniss handelt*/
/* Dabei werden Fehler korrigiert . Sie gibt einen gueltigen Dateistream zurück und setzt den Content Type fest.*/
/* ----------------------------------------------------------------------------------------------------------------------*/

FILE * isfileok (char y[BUFFERSIZE], int * sendtype, char content[MAXCTYPELENGHT])
{
	int a,b=0,c,n,m;
	char x[600];	
	char end[5];
	DIR *pdir;
	FILE *pfile;


	

		
	/* Die Funktion request_type wird aufgerufen, um zu überprüfen, welcher anfragentyp vorliegt*/
	/* Wenn die Anfrage ungültig ist, so wird der Sendetyp 1 - Also GET gewählt, da eine Fehlermeldung */
	/* Übertragen wird */

	c=request_type(y);
	if     (1==c) {a=4; *sendtype=1;}
		else if(2==c) {a=5; *sendtype=2;}
			else *sendtype=1;



	/* Bei einer Gültigen Anfrage, wird der vom Client gesendete Anfrage String Analysiert: Es wird ab dem 4 / 5 Buchstaben*/ 
	/*(HEAD: 5 GET: 4) der String weiter abgearbeitet, bis zu einem Leer- oder Sonderzeichen. Damit hat man den String, der */
	/* (hoffentlich) auf die richtige Datei zeigt.*/   
	 
	if( c==1 || c==2 )
	{ 
		if(a!=4)a=5;	
		for(;
				y[a] != ' '  &&
				y[a] != '\0' &&
				y[a] != '\n' &&
				y[a] != '\r' ;
				a++,b++)
		{

			x[b]=y[a];
		}
		x[b]=0;	

		/* Ueberpruefung, ob es sich um ein Verzeichniss handelt */
		/* Der String wird als Verzeichniss geöffnet. Schlägt dies Fehl, handelt es sich offensichtlich nicht*/
		/* Um ein vorhandenes Verzeichniss */


		pdir=opendir(x);
		printf("CHILD: Try to open %s as directory...\n",x);
		if(0!=pdir)
		{
			printf("CHILD: It is a directory !\n");
			printf("%c\n",x[b]);
			closedir(pdir);
			/* Sollte das Abschließende / bei einem Verzeichnsis Felsen - wird dies hinzugefügt */
			if(x[b]!='/') 
			{
				printf("%s \n",x); 
				printf("CHILD: / is Missing, adding / ... \n"); 
				x[++b]='/';
				x[b+1]=0;
			}

			/*Handelt es sich um ein vorhandenes Verzeichniss so wird der Name der definierten (s.o.) Standartnamens */
			/*angehängt.*/
			printf("CHILD: No Filename given, adding \"%s\"\n",STDFILE);

			for(n=strlen(STDFILE),m=0,b=1;m!=n;m++,b++)
			{
				x[b]=STDFILE[m];


			}
			x[++m]=0;
			printf("CHILD: Now the name is %s \n",x);

		}

	}
	/* Handelt es sich um eine ungültige Anfrage, so wird der Dateistring auf "400.html" (siehe rfc1945) - "Ungültige Anfrage" */
	/* Abgeändert */
	else if(c==-1)
	{ 
		printf("%s\n","Open File 400 - because client did a invalid request"); 
		strcpy(x,"/error/400.html");
		*sendtype=1;
	}



	printf("CHILD: TRY TO OPEN %s\n",x);

	/* Nun wird versucht die Datei zu öffnen */
	pfile=fopen(x,"r");

	/*Filter ob die Datei vorhanden ist*/
	if(0==pfile)
	{

		pfile=fopen("error/404.html","r");

		printf("%s\n %s","CHILD: File not found - Error 404 will be send to Client",x);
		strcpy(x,"error/404.html");	
		*sendtype=1;
	/* Da die Datei nicht vorhanden it, wird die Datei "404" - "File not found" (siehe rfc1945) in den String kopiert.*/
	/* Und der Sendetype auf GET gestellt, da die Fehlermeldung ja übertragen werden muss. */
	}

	/*Filter ob die Fehlerausgabedatei "404 - Datei nicht gefunden" vorhanden ist*/
	if(0==pfile)
	{
		printf("%s\n","CHILD: Error: Required file \"error/404.html\" not aviable !");
		exit(0);

	}

	if(0!=pfile)
	{
		printf("CHILD: File succesfully opened\n");
		/* Das sollte der Regefall sein */
	}


	/* Dateiendungscheck */
	/* Wichtig, um den "Content-Type" zu bestimmen*/

	if(x[strlen(x)-5]=='.')
	{
		end[0]=x[strlen(x)-4];
		end[1]=x[strlen(x)-3];
		end[2]=x[strlen(x)-2];
		end[3]=x[strlen(x)-1];
		end[4]=0;


		if(		(end[0]=='H'||end[0]=='h')
				&&  (end[1]=='T'||end[1]=='t')
				&&  (end[2]=='M'||end[2]=='m')
				&&  (end[3]=='L'||end[3]=='l'))
			strcpy(content,"text/html");

		else if(		(end[0]=='J'||end[0]=='j')
				&&  (end[1]=='P'||end[1]=='p')
				&&  (end[2]=='E'||end[2]=='e')
				&&  (end[3]=='G'||end[3]=='g'))
			strcpy(content,"image/jpeg");
		else 
			strcpy(content,"application/octet-stream");

		printf("4 Zeichen: %c%c%c%c \n",x[strlen(x)-4],x[strlen(x)-3],x[strlen(x)-2],x[strlen(x)-1]);
	}	
	else
	{
		end[0]=x[strlen(x)-3];
		end[1]=x[strlen(x)-2];
		end[2]=x[strlen(x)-1];
		end[3]=0;
		printf("3 Zeichen: %c%c%c   \n",x[strlen(x)-3],x[strlen(x)-2],x[strlen(x)-1]);


		if(	(end[0]=='H'||end[0]=='h')
				&&  (end[1]=='T'||end[1]=='t')
				&&  (end[2]=='M'||end[2]=='m'))
			strcpy(content,"text/html");

		else if(	(end[0]=='T'||end[0]=='t')
				&&  (end[1]=='X'||end[1]=='x')
				&&  (end[2]=='T'||end[2]=='t'))
			strcpy(content,"text/plain");

		else if(	(end[0]=='G'||end[0]=='g')
				&&  (end[1]=='I'||end[1]=='i')
				&&  (end[2]=='F'||end[2]=='f'))
			strcpy(content,"image/gif");

		else if(	(end[0]=='H'||end[0]=='h')
				&&  (end[1]=='T'||end[1]=='t')
				&&  (end[2]=='M'||end[2]=='m'))
			strcpy(content,"text/html");

		else if(	(end[0]=='J'||end[0]=='j')
				&&  (end[1]=='P'||end[1]=='p')
				&&  (end[2]=='G'||end[2]=='g'))
			strcpy(content,"image/jpg");


		else if(	(end[0]=='P'||end[0]=='p')
				&&  (end[1]=='N'||end[1]=='n')
				&&  (end[2]=='G'||end[2]=='g'))
			strcpy(content,"image/png");      
		/* Ist keine bekannte Dateiendung dabei, wird es als Binäre Datei beschrieben - die vom benutzer */
		/* Heruntergeladen werden kann*/

		else strcpy(content,"application/octet-stream");
	}

	printf("Content Type: %s\n",content);
	return(pfile);
}

/* Verschickt den Header und eventuell (bei einer GET Anfrage) die Datei*/

int sendit(FILE * pfile, int socket_out, int type, char content[15])
{
	int much;
	char tosend[BUFFERSIZE]="\0";
	char header[180]="";

	/*Vorerst nur HTML*/


	strcpy(header,"HTTP/0.9 200 OK\r\n");
	strcat(header,"Server: ");
	strcat(header,SERVERNAME);
	strcat(header,SERVERVER);
	strcat(header,"\r\nConnection: close");
	strcat(header,"\r\nContent-Type: ");
	strcat(header,content);
	strcat(header,"\r\n\r\n");


	printf("\nKIND: Sende Header: %s \n___\n Das war der Header\n",header);



	write(socket_out, header, (strlen(header)));
	/**/	/*write(socket_out,"Content-type: text/html\n\n",sizeof("Content-type: text/html\n\n"));
		 */
	/*	 */

	printf("Type ist %d\\n",type);

	if(1==type){
		printf("\nKIND: Sende Datei: \n");

		while( ! feof (pfile) )
		{

			much=fread(tosend,1,sizeof(tosend),pfile);
			/*fscanf(pfile, "%1024s" ,tosend);*/
			printf("%s",tosend);
			if (0 > write(socket_out,tosend,much))
			{
				perror("write");
				return(-1);
			}



		}}
		printf("\n");
		return(1);
}






int main() 
{


	int msgsock;
	struct sockaddr_in address; 	/* Adresse des Servers wird festgelegt*/
	struct sockaddr_in fremdrechner; /* Adresse des Verbindungspartners*/
	unsigned int fremdlenght;		/* Die Länge der Adresse des Fremdrechners*/ 
	int sock; 			/* Ein Socket, der benutzt wird.*/
	int n;
	int send_type=0;
	char content_type[MAXCTYPELENGHT];
	FILE *pfile;

	/*Festlegung der Standardeinstellungen*/


	address.sin_addr.s_addr= INADDR_ANY; /*inet_addr("127.0.0.1");*/ /*IP Adresse auf der gelauscht werden soll - vorerst alle^*/
	address.sin_port =  htons (PORT); /*Welcher Port - niedrige duerfen nur von root geoeffnet werden*/
	address.sin_family = AF_INET; /*Welche Protokollfamilie*/


	chroot(STDDIR);
	/*chroot("/home/ppweissm/webserver/test/");*/
	printf("%s","Anfang des Programmes\n");

	sock = socket(AF_INET,SOCK_STREAM,0); /*Ein Socket wird geöffnet*/



	if( -1 == sock )
		perror("socket"); /*Überprüfung ob der Socket offen ist*/
	else
		printf("%s","Socket erfolgreich geöffnet\n");

	printf("%s","Socket wird auf LISTEN gestellt...\n");
	if (-1 ==bind(sock,((void *) &address),sizeof(address))) /*Bindet den Socket an den Port*/
	{printf("%s","Bindefehler\n");perror("bind");exit(0);}
	printf("%s","Alles gebunden\n");
	/*listen(sock,5);*/
	while (1)
	{

		/*int msgsock;*/ /*Socketnachricht;*/
		printf("%s","VATER: Lauschen\n");	    
		if (0 > listen(sock,1)) /* Es wird "gelauscht"*/
			perror("listen"); /*"Lauschen" schlug fehl*/
		printf("%s","VATER: Socket annehmen\n");

		fremdlenght=sizeof(fremdrechner);





		if (0 > (msgsock = accept(sock, (struct sockaddr *)&fremdrechner,&fremdlenght))) /*Annehmen einer verbindung schlug fehl*/
			perror("accept");


		printf("%s %s %s\n","VATER: Verbindung von Rechner " ,inet_ntoa(fremdrechner.sin_addr) ," - ich bekomme ein Kind");
		waitpid(-1,NULL,WNOHANG);
		if(-1 == (pid = fork()))
		{
			perror ("fork");
			printf("%s"," \"Fehlgeburt\" \n");
			exit(0);
		}
		else if(0 == pid)
		{
			char puffer[BUFFERSIZE]="";
			/*Der Kindprozess*/


			printf("CHILD: Client requests:\n");
			while((n=read(msgsock,puffer,sizeof(puffer)) < 0))
			{ 
				puffer[n]=0;
				printf("%s\n",puffer); /*Das hat der Andere gesagt*/ 
			}


			fflush(stdout);
			pfile=isfileok(puffer,&send_type,content_type);

			sendit(pfile,msgsock,send_type,content_type);

			printf("%s \n","KIND: Verbindung zumachen");

			if (0 > close (msgsock))
				perror("close");
			printf("%s\n","KIND: Die Verbindung ist zu - Saionara !");
			exit(0);
		}
		else{close(msgsock);}
	}
	if(0 > close(sock))
		perror("close");

}

