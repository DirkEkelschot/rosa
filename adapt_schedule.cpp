#include "adapt_schedule.h"




ScheduleObj* DoScheduling(std::map<int,std::vector<int> > Rank2RequestEntity, MPI_Comm comm)
{
    int i,t;
    int size;
    MPI_Comm_size(comm, &size);
    // Get the rank of the process
    int rank;
    MPI_Comm_rank(comm, &rank);
    
    int nRank_RequestEntity               = Rank2RequestEntity.size(); // The number of ranks from which current rank requests vertices/faces/elements.
    int* reduced_nRank_RequestEntity = new int[size]; // Defined memory for a reduced array so that all ranks are going to be aware of which information is required from each other.
    int* arr_nRank_RequestEntity     = new int[size]; // Defining memory to store local requesting information.

    for(i=0;i<size;i++)
    {
        reduced_nRank_RequestEntity[i] = 0;
        
        if(i==rank)
        {
            arr_nRank_RequestEntity[i] = nRank_RequestEntity+1;
        }
        else
        {
            arr_nRank_RequestEntity[i] = 0;
        }
    }

    // This array hold the information so that each rank knows from how many ranks other ranks request entities.
    MPI_Allreduce(arr_nRank_RequestEntity, reduced_nRank_RequestEntity, size, MPI_INT, MPI_SUM, comm);


    int* reduced_nRank_RequestEntity_offset = new int[size];// Define an offset array for the schedule in order to be able to gather the local schedules for each rank to a global schedule array.

    int offset = 0;
    for(i=0;i<size;i++)
    {
        reduced_nRank_RequestEntity_offset[i] = offset;
        offset = offset+reduced_nRank_RequestEntity[i];
    }
    int nTot_RequestEntity = 0;
    int nRank_RequestEntity_p_one = nRank_RequestEntity+1; // This size is added by one since we add the current rank number to the array.

    MPI_Allreduce(&nRank_RequestEntity_p_one, &nTot_RequestEntity, 1, MPI_INT, MPI_SUM, comm);// Determine the total length of the "schedule" array.
    int* sendFromRank2Rank_Entity_Global = new int[nTot_RequestEntity]; // This array is laid out as follows:
    // first the fromRank is noted which is followed by the IDs for the several ranks FromRank is requesting entities.
    int* sendNentityFromRank2Rank_Global = new int[nTot_RequestEntity]; // This array is layout as follows:
    // first the fromRank is noted which is followed by the number of entities is listed for the several ranks FromRank is sending to.

    for(i=0;i<nTot_RequestEntity;i++)
    {
        sendFromRank2Rank_Entity_Global[i]   = 0;
        sendNentityFromRank2Rank_Global[i]   = 0;
    }

    int* ReqRank_fromRank_Entity   = new int[nRank_RequestEntity+1];
    int* ReqNentity_from_rank      = new int[nRank_RequestEntity+1];
    ReqRank_fromRank_Entity[0]     = rank;
    ReqNentity_from_rank[0]        = -1;

    t = 1;
    std::map<int,std::vector<int> >::iterator it2;
    for(it2=Rank2RequestEntity.begin();it2!=Rank2RequestEntity.end();it2++)
    {
        ReqRank_fromRank_Entity[t] = it2->first;
        ReqNentity_from_rank[t]    = it2->second.size();
        t++;
    }

    /* The example could be:
       rank 0 sends element 1 and 2 to rank 1 and element 10 to rank 2
       rank 1 sends element 2 to rank 0 and element 5 to rank 3
       rank 2 send element 4 to rank 0 and element 3 to rank 3
       rank 3 send element 9 to rank 1 and element 5, 7 and 8 to rank 2

       Then sendFromRank2Rank_Entity_Global and sendNentityFromRank2Rank_Global will look like:

       sendFromRank2Rank_Entity_Global[0]  = 0  sendNentityFromRank2Rank_Global[0]  = -1
       sendFromRank2Rank_Entity_Global[1]  = 1  sendNentityFromRank2Rank_Global[0]  =  2
       sendFromRank2Rank_Entity_Global[2]  = 2  sendNentityFromRank2Rank_Global[0]  =  1
       sendFromRank2Rank_Entity_Global[3]  = 1  sendNentityFromRank2Rank_Global[0]  = -1
       sendFromRank2Rank_Entity_Global[4]  = 0  sendNentityFromRank2Rank_Global[0]  =  1
       sendFromRank2Rank_Entity_Global[5]  = 3  sendNentityFromRank2Rank_Global[0]  =  1
       sendFromRank2Rank_Entity_Global[6]  = 2  sendNentityFromRank2Rank_Global[0]  = -1
       sendFromRank2Rank_Entity_Global[7]  = 0  sendNentityFromRank2Rank_Global[0]  =  1
       sendFromRank2Rank_Entity_Global[8]  = 3  sendNentityFromRank2Rank_Global[0]  =  1
       sendFromRank2Rank_Entity_Global[9]  = 3  sendNentityFromRank2Rank_Global[0]  = -1
       sendFromRank2Rank_Entity_Global[10] = 1  sendNentityFromRank2Rank_Global[0]  =  1
       sendFromRank2Rank_Entity_Global[11] = 2  sendNentityFromRank2Rank_Global[0]  =  3
    */

    MPI_Allgatherv(&ReqRank_fromRank_Entity[0],
                   nRank_RequestEntity_p_one, MPI_INT,
                   &sendFromRank2Rank_Entity_Global[0],
                   reduced_nRank_RequestEntity, reduced_nRank_RequestEntity_offset,
                   MPI_INT,comm);

    MPI_Allgatherv(&ReqNentity_from_rank[0],
                   nRank_RequestEntity_p_one, MPI_INT,
                   &sendNentityFromRank2Rank_Global[0],
                   reduced_nRank_RequestEntity, reduced_nRank_RequestEntity_offset,
                   MPI_INT,comm);


    ScheduleObj* scheduleObj   = new ScheduleObj;
    
    std::map<int, std::set<int> >    sendFromRank2Rank_v_set;
    std::map<int, std::set<int> >    recvRankFromRank_map_v_set;
    
    
    //============================================================================
    for(i=0;i<size;i++)
    {
        int of = reduced_nRank_RequestEntity_offset[i];
        int nl = reduced_nRank_RequestEntity[i];
        
        for(int j=of+1;j<of+nl;j++)
        {
            scheduleObj->SendFromRank2Rank[sendFromRank2Rank_Entity_Global[of]].insert(sendFromRank2Rank_Entity_Global[j]);
            
            scheduleObj->RecvRankFromRank[sendFromRank2Rank_Entity_Global[j]].insert(sendFromRank2Rank_Entity_Global[of]);
        }
    }
    //============================================================================
    delete[] reduced_nRank_RequestEntity_offset;
    delete[] sendNentityFromRank2Rank_Global;
    delete[] sendFromRank2Rank_Entity_Global;
    delete[] ReqRank_fromRank_Entity;
    delete[] ReqNentity_from_rank;
    delete[] reduced_nRank_RequestEntity;
    delete[] arr_nRank_RequestEntity;

    return scheduleObj;
    
}
