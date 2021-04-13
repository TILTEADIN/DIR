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
#define L 3

int read_file(float sides[]);
void greet_neighbours(int rank, int neighbours[]);
float minimum(int rank,int neighbours[],float node_buffer);

int main(int argc, char *argv[]){

    int rank, size; 
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int readed_n,err_check;
    float sides[L*L];
    float node_buffer,node_minimum;
    int neighbours[4]; //Ranked from 0 to 3: North, South,East,West
    MPI_Status status;
    MPI_Request request;
    

    if(rank ==0){
        if(size!=L*L){
            err_check = 0; // If dimensions don't match, all proceses shall be terminated through a multicast message. This is caused by a bad -n number while using mpirun
            fprintf(stderr,"Number of nodes (%d) doesn't match with toroid neighbours (%d)\nAborting\n",size,L*L);
            MPI_Bcast(&err_check, 1, MPI_INT,0, MPI_COMM_WORLD);

        }else{

            readed_n = read_file(sides); 

            if(size!=readed_n){ // If the amount of data prodived in datos.dat doesn't match the number of neighbours/nodes prints an error
                err_check=0; 
                fprintf(stderr,"Number of nodes(%d) doesn't match with provided data (%d)\nAborting\n",size,readed_n);
                MPI_Bcast(&err_check,1, MPI_INT,0, MPI_COMM_WORLD);

            }else{

                for (int i=0;i< readed_n;i++){
                    node_buffer = sides[i];
                    MPI_Send(&node_buffer,1,MPI_FLOAT,i,0,MPI_COMM_WORLD); //Rank 0 distributes to each and every node the data that has been read
                }
                err_check = 1; //Rank 0 sends a message allowing every neighbour/node to continue
                MPI_Bcast(&err_check,1,MPI_INT,0, MPI_COMM_WORLD);
            }
        }            
    }

    MPI_Bcast(&err_check, 1, MPI_INT, 0, MPI_COMM_WORLD); // Each node receives a message from node 0 to check wheter message passing has been succesfull

    if(err_check==1){

        MPI_Recv(&node_buffer,1,MPI_FLOAT,0,0,MPI_COMM_WORLD,&status);
        greet_neighbours(rank,neighbours); //Obtains every node neighbours
        node_minimum = minimum(rank,neighbours,node_buffer); // Obtaining node minimum

        if (rank==0){

            printf("Minimum value is: %3f\n",node_minimum); //Rank 0 will show the networks minimum vale
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
    int row = rank/L;
    int column = rank%L;


    switch (row){ // North->0 y South->1
        
        case 0:
            neighbours[0] = rank+L;
            neighbours[1] = (L*(L-1))+rank;
            break;

        case L-1:
            neighbours[0] = column;
            neighbours[1] = rank-L;
            break;

        default:
            neighbours[0] = rank+L;
            neighbours[1] = rank-L;
            break;
    }

    switch (column){ //East-> 2 & West -> 3
    
        case 0:
            neighbours[2] = rank+1;
            neighbours[3] = rank+(L-1);
            break;

        case L-1:
            neighbours[2] = rank-(L-1);
            neighbours[3] = rank-1;
            break;
        default:
            neighbours[2] = rank+1;
            neighbours[3] = rank-1;
            break;
    }

}
float minimum(int rank,int neighbours[],float node_buffer){
    
    MPI_Status status;
    MPI_Request request;
    float min_rank;
    min_rank = node_buffer; 
        for (int j=0;j<L;j++){

            
            MPI_Isend(&min_rank,1,MPI_FLOAT,neighbours[0],10,MPI_COMM_WORLD,&request); //Sending North neighbours the buffer's value

            MPI_Recv(&node_buffer,1,MPI_FLOAT,neighbours[1],10,MPI_COMM_WORLD,&status); //Receiving calculated values from South neighbours
            
            
            MPI_Wait(&request,&status); //Waiting until values have been copied properly thus making the buffer reusable 
            if(node_buffer<min_rank){   //Comparing values and storing the minimum one
                min_rank = node_buffer;
            }
           
        }

        for (int j=0;j<L;j++){

        
            MPI_Isend(&min_rank,1,MPI_FLOAT,neighbours[2],32,MPI_COMM_WORLD,&request); //Sending East neighbours the buffer's value

            MPI_Recv(&node_buffer,1,MPI_FLOAT,neighbours[3],32,MPI_COMM_WORLD,&status); //Receiving calculated values from West neighbours
            
            MPI_Wait(&request,&status); //Waiting until values have been copied properly thus making the buffer reusable 
            if(node_buffer<min_rank){   //Comparing values and storing the minimum one
                min_rank = node_buffer;
            }
            
        }
        return min_rank;

} 