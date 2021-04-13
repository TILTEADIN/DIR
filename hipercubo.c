#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include "mpi.h"

#define file "datos.dat"
#define MAX_BUFFER 1024
#define D 3



int read_file(float sides[]);
void greet_neighbours(int rank, int neighbours[]);
float maximum(int rank,int neighbours[],float node_buffer);

int main(int argc, char *argv[]){

    int rank, size; 
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int readed_n,err_check,dimension;
    dimension = pow(2,D); 
    float sides[dimension]; 
    float buffer, node_max;
    int neighbours[D]; 
    MPI_Status status;
    MPI_Request request;


    if(rank ==0){
        if(size!=dimension){
            err_check = 0; // If dimensions don't match, all proceses shall be terminated through a multicast message. This is caused by a bad -n number while using mpirun
            fprintf(stderr,"Number of nodes (%d) doesn't match with hypercube neighbours (%d)\nAborting\n",size,dimension);
            MPI_Bcast(&err_check, 1, MPI_INT,0, MPI_COMM_WORLD);

        }else{

            readed_n = read_file(sides); 
            if(size!=readed_n){ // If the amount of data prodived in datos.dat doesn't match the number of neighbours/nodes prints an error
                err_check=0; 
                fprintf(stderr,"Number of nodes(%d) doesn't match with provided data (%d)\nAborting\n",size,readed_n);
                MPI_Bcast(&err_check,1, MPI_INT,0, MPI_COMM_WORLD);

            }else{

                for (int i=0;i<readed_n;i++){
                    buffer = sides[i];
                    MPI_Send(&buffer,1,MPI_FLOAT,i,0,MPI_COMM_WORLD); //Rank 0 distributes to each and every node the data that has been read                      

                }
                err_check = 1; //Rank 0 sends a message allowing every neighbour/node to continue
                MPI_Bcast(&err_check,1,MPI_INT,0, MPI_COMM_WORLD);
            }
        }            
    }
    
    MPI_Bcast(&err_check, 1, MPI_INT, 0, MPI_COMM_WORLD); // Each node receives a message from node 0 to check wheter message passing has been succesfull

    if(err_check==1){

        MPI_Recv(&buffer,1,MPI_FLOAT,0,0,MPI_COMM_WORLD,&status); 
        greet_neighbours(rank,neighbours);  //Obtains every node neighbours
        node_max = maximum(rank,neighbours,buffer); // Obtaining node maximum
        
        if (rank==0){

            printf("Maximum value is: %3f\n",node_max); //Rank 0 will show the networks maximum vale
        }
        
    }
    MPI_Finalize();
    return 0;
}

int read_file(float number_list[]){
    
    FILE* fichero;
    char *buffer = malloc(MAX_BUFFER*sizeof(char));
    int num_sides=0;
    char *token;
    fichero = fopen(file,"r");

    if (fichero == NULL){
        fprintf(stderr,"Error whilst reading file %s\n",file);
        exit(EXIT_FAILURE);
    }
    while (fscanf(fichero,"%s",buffer)!=EOF)
    {
    }
    fclose(fichero);

    token = strtok(buffer,",");
    while(token != NULL ) {
      
      number_list[num_sides] = atof(token);
      token = strtok(NULL,",");
      num_sides++;
    }
    return num_sides;

}

void greet_neighbours(int rank,int neighbours[]){

    for(int i=0;i<D;i++){
        neighbours[i] = rank ^ (1 << i);  //Each node corresponding neighbours is obtained by using a logical shift
    
    }
}

float maximum (int rank, int neighbours[],float node_buffer){
    float node_max;
    MPI_Status status;
    MPI_Request request;
    node_max = node_buffer; // Storing each nodes maximum value
    for (int i=0;i<D;i++){ //Iterating over neighbours 

        MPI_Isend(&node_max,1,MPI_FLOAT,neighbours[i],D,MPI_COMM_WORLD,&request); //Sending the buffer's value

        MPI_Recv(&node_buffer,1,MPI_FLOAT,neighbours[i],D,MPI_COMM_WORLD,&status); //Receiving the value sent by neighbours
                
        MPI_Wait(&request,&status); //Waiting until values have been copied properly thus making the buffer reusable 
        if(node_buffer>node_max){ //Comparing values and storing the maximum one
            node_max = node_buffer;
        }
            
    }
        return node_max; 

}
