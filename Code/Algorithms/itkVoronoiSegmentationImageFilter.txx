/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    itkVoronoiSegmentationImageFilter.txx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2001 Insight Consortium
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * The name of the Insight Consortium, nor the names of any consortium members,
   nor of any contributors, may be used to endorse or promote products derived
   from this software without specific prior written permission.

  * Modified source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#ifndef _itkVoronoiSegmentationImageFilter_txx
#define _itkVoronoiSegmentationImageFilter_txx

#include "itkImageRegionIteratorWithIndex.h"


namespace itk
{

/* constructor: seting the default value of the parameters */
template <class TInputImage, class TOutputImage>
VoronoiSegmentationImageFilter <TInputImage,TOutputImage>::
VoronoiSegmentationImageFilter(){
  m_MinRegion = 20;
  m_Steps = 0;
  m_LastStepSeeds = 0;
  m_NumberOfSeeds = 200;
  m_MeanPercentError = 0.10;
  m_MeanDeviation = 0.8;
  m_VarPercentError = 1.5;
  m_UseBackgroundInAPrior = 1;
}

/* destructor */
template <class TInputImage, class TOutputImage>
VoronoiSegmentationImageFilter <TInputImage,TOutputImage>::
~VoronoiSegmentationImageFilter(){
}

template <class TInputImage, class TOutputImage>
void
VoronoiSegmentationImageFilter <TInputImage,TOutputImage>::
SetMeanPercentError(double x){
  m_MeanPercentError = x;
  m_MeanTolerance = x*m_Mean;
}


template <class TInputImage, class TOutputImage>
void
VoronoiSegmentationImageFilter <TInputImage,TOutputImage>::
SetVarPercentError(double x){
  m_VarPercentError = x;
  m_VarTolerance = x*m_Var;
}


/* initialization for the segmentation */
template <class TInputImage, class TOutputImage>
void
VoronoiSegmentationImageFilter <TInputImage,TOutputImage>::
InitializeSegment(void){
  m_InputImage = this->GetInput();
  m_OutputImage = this->GetOutput(); 

  m_Size = m_InputImage->GetLargestPossibleRegion().GetSize();
  IndexType index = IndexType::ZeroIndex;
  RegionType region;
  region.SetSize(m_Size);
  region.SetIndex(index);
  m_OutputImage->SetLargestPossibleRegion( region );
  m_OutputImage->SetBufferedRegion( region );
  m_OutputImage->SetRequestedRegion( region );
  m_OutputImage->Allocate();  

  m_WorkingVD=VoronoiDiagram::New();
  VoronoiDiagram::PointType VDsize;
  VDsize[0] = (VoronoiDiagram::CoordRepType)(m_Size[0]-0.1);
  VDsize[1] = (VoronoiDiagram::CoordRepType)(m_Size[1]-0.1);
  m_WorkingVD->SetBoundary(VDsize);
  m_WorkingVD->SetRandomSeeds(m_NumberOfSeeds);
  m_StepsRuned = 0;
}

template <class TInputImage, class TOutputImage>
void
VoronoiSegmentationImageFilter <TInputImage,TOutputImage>::
GetStats(PointTypeDeque vertlist, double *savemean, double *savevar, int *nump)
{

  int num=0;
  double addp=0;
  double addpp=0;

  IndexType idx;
  float getp;

  PointType currP;
  PointType leftP;
  PointType rightP;
  currP = vertlist.front();
  vertlist.pop_front();
  leftP = vertlist.front();
  while(currP[1] > leftP[1]){
    vertlist.push_back(currP);
	currP=vertlist.front();
    vertlist.pop_front();
	leftP=vertlist.front();
  }
  rightP=vertlist.back();
  while(currP[1] > rightP[1]){
    vertlist.push_front(currP);
	currP=vertlist.back();
	vertlist.pop_back();
	rightP=vertlist.back();
  }
  leftP=vertlist.front();


  int vsize = vertlist.size();
  double beginx;
  double beginy;
  double endx;
  double endy;
  double leftendy;
  double rightendy;
  double offset;
  double leftDx;
  double rightDx;
  bool RorL;
  bool saveRorL;
  int intbeginx;
  int intendx;
  int intbeginy;
  int intendy;
  int i;
  int j;
  
  beginx = currP[0];
  endx = currP[0];
  beginy = currP[1];
  leftendy = leftP[1];
  rightendy = rightP[1];
  if(leftendy > rightendy){
    RorL = 1;
	endy = rightendy;
  }
  else{
    RorL = 0;
	endy = leftendy;
  }
  intbeginy = (int)(beginy);
  intendy = (int)endy;
  if(intbeginy == intendy){
    if(RorL){
      leftDx = (leftP[0]-beginx)/(leftP[1]-beginy);
    }
    else{
      rightDx = (rightP[0]-beginx)/(rightP[1]-beginy);
    }    
  }
  else{
    leftDx = (leftP[0]-beginx)/(leftP[1]-beginy);
    rightDx = (rightP[0]-beginx)/(rightP[1]-beginy);
	intbeginy++;
    offset = (double)intbeginy - beginy;  
    if(RorL){
	  endx += offset*rightDx;
	}
	else{
	  beginx += offset*leftDx;
	}
    if(beginx < endx){
	  for(i = intbeginy;i<=intendy;i++){
	    intbeginx = (int)(beginx+1);
	    intendx = (int)endx;
	    idx[1]=i;
	    for(j = intbeginx; j < intendx; j++){
          num++;
		  idx[0]=j;
		  getp=(float)(m_InputImage->GetPixel(idx));
		  addp+=getp;
		  addpp+=getp*getp;
        }
  	    beginx += leftDx;
	    endx += rightDx;
	  }
    }		  
    else{
	  for(i = intbeginy;i<=intendy;i++){
	    intendx = (int)(beginx+1);
	    intbeginx = (int)endx;
	    idx[1]=i;
	    for(j = intbeginx; j < intendx; j++){
          num++;
  		  idx[0]=j;
		  getp=(float)(m_InputImage->GetPixel(idx));
		  addp+=getp;
		  addpp+=getp*getp;
        }
	    beginx += leftDx;
	    endx += rightDx;
	  }
    }		  
  }

  while(vsize > 1){
    vsize--;
    if(RorL){
	  endx = rightP[0];
	  beginy = rightP[1];
	  vertlist.pop_back();
	  rightP = vertlist.back();
      rightDx = (rightP[0]-endx)/(rightP[1]-beginy);
	}
	else{
	  beginx = leftP[0];
	  beginy = leftP[1];
	  vertlist.pop_front();
	  leftP = vertlist.front();
      leftDx = (leftP[0]-beginx)/(leftP[1]-beginy);
	}
	leftendy = leftP[1];
	rightendy = rightP[1];
	if(leftendy > rightendy){
	  saveRorL = 1;
	  endy = rightendy;
	}
	else{
	  saveRorL = 0;
	  endy = leftendy;
	}
	
	intbeginy = (int)(beginy);
	intendy = (int)(endy);
	if(intbeginy == intendy){
      i = i; //donothing;
	}
	else{
	  intbeginy++;
	  offset = (double)intbeginy - beginy;  
      if(RorL){
	    endx += offset*rightDx;
	  }
	  else{
	    beginx += offset*leftDx;
	  }
    }
    if(beginx < endx){
	  for(i = intbeginy;i<=intendy;i++){
	    intbeginx = (int)(beginx+1);
		intendx = (int)endx;
	    idx[1]=i;
		for(j = intbeginx; j < intendx; j++){
        num++;
		idx[0]=j;
		getp=(float)(m_InputImage->GetPixel(idx));
		addp+=getp;
		addpp+=getp*getp;
        }
		beginx += leftDx;
		endx += rightDx;
	  }
	}		  
    else{
	  for(i = intbeginy;i<=intendy;i++){
	    intendx = (int)(beginx+1);
		intbeginx = (int)endx;
	    idx[1]=i;
		for(j = intbeginx; j < intendx; j++){
        num++;
		idx[0]=j;
		getp=(float)(m_InputImage->GetPixel(idx));
		addp+=getp;
		addpp+=getp*getp;
        }
		beginx += leftDx;
		endx += rightDx;
	  }
	}		  
	RorL = saveRorL;
  }

  (*nump) = num;
  if(num > 1){
    (*savemean) = addp/num;
    (*savevar) = sqrt(addpp - (addp*addp)/(num) )/(num-1);
  }
  else{
    (*savemean) = 0;
    (*savevar) = 0;
  }  
}


template <class TInputImage, class TOutputImage>
void
VoronoiSegmentationImageFilter <TInputImage,TOutputImage>::
ClassifyDiagram(void)
{
  CellPointer currCell; 
  double currMean;
  double currVar;
  int totalnum;
  PointIdIterator currPit;
  PointIdIterator currPitEnd;
  PointType currP;
  PointTypeDeque VertList;
  double savem;
  double savev;
  for(int i=0;i<m_NumberOfSeeds;i++){
    currCell = m_WorkingVD->GetCellId(i);
	currPitEnd = currCell->PointIdsEnd();
	VertList.clear();
	for(currPit=currCell->PointIdsBegin();currPit!=currPitEnd;++currPit){
	  m_WorkingVD->GetPointId((*currPit),&(currP));
	  VertList.push_back(currP);
	}
	GetStats(VertList,&currMean,&currVar,&totalnum);
	m_NumberOfPixels[i] = totalnum;
    savem = currMean - m_Mean;
	savev = currVar - m_Var;
	if( (savem>-m_MeanTolerance) && (savem<m_MeanTolerance) && (savev<m_VarTolerance) )
	  m_Label[i] = 1;
    else
	  m_Label[i] = 0;
  }

  m_NumberOfBoundary = 0;
  for(int i=0;i<m_NumberOfSeeds;i++){
    if(m_Label[i] == 0){
	  NeighborIdIterator itend = m_WorkingVD->NeighborIdsEnd(i);
	  NeighborIdIterator it=m_WorkingVD->NeighborIdsBegin(i);
	  bool bnd = 0;
	  while((it != itend) && (!bnd)){
	    bnd = (m_Label[*it] == 1);
		++it;
	  }
	  if(bnd){
	    m_Label[i] = 2;
		m_NumberOfBoundary++;
	  }
	}
  }
}

template <class TInputImage, class TOutputImage>
void
VoronoiSegmentationImageFilter <TInputImage,TOutputImage>::
GenerateAddingSeeds(void)
{
  EdgeIterator eit;
  EdgeIterator eitend =	m_WorkingVD->EdgeEnd();  
  PointType adds;
  Point<int,2> seeds;

  for(eit = m_WorkingVD->EdgeBegin();eit != eitend; ++eit){
	seeds = m_WorkingVD->GetSeedsIDAroundEdge(&*eit);
	if( ((m_Label[seeds[0]]==2)||(m_Label[seeds[1]]==2))
	  && (m_NumberOfPixels[seeds[0]]>m_MinRegion)
	  && (m_NumberOfPixels[seeds[1]]>m_MinRegion) ){
	  adds[0] = (eit->m_left[0] + eit->m_right[0])*0.5;
	  adds[1] = (eit->m_left[1] + eit->m_right[1])*0.5;
	  m_SeedsToAdded.push_back(adds);
	}
  }
}


template <class TInputImage, class TOutputImage>
void
VoronoiSegmentationImageFilter <TInputImage,TOutputImage>::
ExcuteSegmentOneStep(void){
  m_NumberOfPixels.resize(m_NumberOfSeeds);
  m_Label.resize(m_NumberOfSeeds);
  m_SeedsToAdded.clear();
  m_WorkingVD->GenerateDiagram();
  ClassifyDiagram();
  GenerateAddingSeeds();
  m_NumberOfSeedsToAdded = m_SeedsToAdded.size();
  m_StepsRuned++;
}

template <class TInputImage, class TOutputImage>
void
VoronoiSegmentationImageFilter <TInputImage,TOutputImage>::
ExcuteSegment(void){
  bool ok = 1;
  if(m_Steps == 0){
    ExcuteSegmentOneStep();
	if(m_NumberOfBoundary == 0){
	  ok=0;
    }
    while( (m_NumberOfSeedsToAdded != 0) && ok){
	  m_WorkingVD->AddSeeds(m_NumberOfSeedsToAdded,m_SeedsToAdded.begin());
	  m_LastStepSeeds = m_NumberOfSeeds;
	  m_NumberOfSeeds += m_NumberOfSeedsToAdded;
      ExcuteSegmentOneStep();
	}
  }
  else if(m_Steps == 1){
    ExcuteSegmentOneStep();
  }
  else{
    ExcuteSegmentOneStep();
	if(m_NumberOfBoundary == 0){
	  ok=0;
    }
	int i = 1;
	while((i<m_Steps) && ok){
	  m_WorkingVD->AddSeeds(m_NumberOfSeedsToAdded, m_SeedsToAdded.begin());
	  m_LastStepSeeds = m_NumberOfSeeds;
	  m_NumberOfSeeds += m_NumberOfSeedsToAdded;
      ExcuteSegmentOneStep();
	  i++;
	}
  }	  
  MakeSegmentBoundary();
}

template <class TInputImage, class TOutputImage>
void
VoronoiSegmentationImageFilter <TInputImage,TOutputImage>::
BeforeNextStep(void){
  m_WorkingVD->AddSeeds(m_NumberOfSeedsToAdded, m_SeedsToAdded.begin());
  m_LastStepSeeds = m_NumberOfSeeds;
  m_NumberOfSeeds += m_NumberOfSeedsToAdded;
}


template <class TInputImage, class TOutputImage>
void
VoronoiSegmentationImageFilter <TInputImage,TOutputImage>::
drawLine(PointType p1,PointType p2){
  int x1=(int)(p1[0]+0.5);
  int x2=(int)(p2[0]+0.5);
  int y1=(int)(p1[1]+0.5);
  int y2=(int)(p2[1]+0.5);
  if(x1==m_Size[0]) x1--;  
  if(x2==m_Size[0]) x2--;  
  if(y1==m_Size[1]) y1--;  
  if(y2==m_Size[1]) y2--;  

  int dx=x1-x2;
  int adx=(dx>0)?dx:-dx;
  int dy=y1-y2;
  int ady=(dy>0)?dy:-dy;
  int save;
  float curr;
  IndexType idx;
  if (adx > ady){
    if(x1>x2){
	  save=x1; x1=x2; x2=save;
	  save=y1; y1=y2; y2=save;
	}
    curr=(float)y1;
    float offset=(float)dy/dx;
	for(int i=x1;i<=x2;i++){
	  idx[0]=i;
	  idx[1]=y1;
	  m_OutputImage->SetPixel(idx,1);
	  curr += offset;
  	  y1=(int)(curr+0.5);
    }
  }
  else { 
    if(y1>y2){
	  save=x1; x1=x2; x2=save;
	  save=y1; y1=y2; y2=save;
	}
    curr=(float)x1;
    float offset=(float)dx/dy;
	for(int i=y1;i<=y2;i++){
	  idx[0]=x1;
	  idx[1]=i;
	  m_OutputImage->SetPixel(idx,1);
	  curr += offset;
	  x1=(int)(curr+0.5);
    }
  }

}

template <class TInputImage, class TOutputImage>
void
VoronoiSegmentationImageFilter <TInputImage,TOutputImage>::
TakeAPrior(BinaryObjectImage* aprior)
{
  RegionType region = m_InputImage->GetRequestedRegion();
  itk::ImageRegionIteratorWithIndex <BinaryObjectImage> ait(aprior, region);
  itk::ImageRegionIteratorWithIndex <InputImageType> iit(m_InputImage, region);

  int num=0;
  float addp=0;
  float addpp=0;
  float currp;

  int i,j;
  int minx,miny,maxx,maxy;
  bool status=0;
  for(i=0;i<m_Size[1];i++){
    for(j=0;j<m_Size[0];j++){
       if( (status==0)&&(ait.Get()) ){
         miny=i;
         minx=j;
         maxy=i;
         maxx=j;
         status=1;
       } 
       else if( (status==1)&&(ait.Get()) ){
         maxy=i;
         if(minx>j) minx=j;
         if(maxx<j) maxx=j;
       }  
    ++ait;
    }
  }       
  
  float addb=0;
  float addbb=0;
  int numb=0;

  ait.GoToBegin();
  iit.GoToBegin();
  for(i=0;i<miny;i++){
    for(j=0;j<m_Size[0];j++){
      ++ait;
      ++iit;
    }
  }
  for(i=miny;i<=maxy;i++){
    for(j=0;j<minx;j++){
      ++ait;
      ++iit;
    }
    for(j=minx;j<=maxx;j++){
      if(ait.Get()){
	    num++;
	    currp = (float)(iit.Get());
	    addp += currp;
	    addpp += currp*currp;
	  }
      else{
	    numb++;
	    currp = (float)(iit.Get());
	    addb += currp;
	    addbb += currp*currp;
	  }
      ++ait;++iit;
    }
    for(j=maxx+1;j<m_Size[0];j++){
      ++ait;
      ++iit;
    }
  }

  m_Mean = addp/num;
  m_Var = sqrt((addpp - (addp*addp)/num)/(num-1));
  float b_Mean = addb/numb;
  float b_Var = sqrt((addbb - (addb*addb)/numb)/(numb-1));
  if(m_UseBackgroundInAPrior)
    m_MeanTolerance = fabs(m_Mean-b_Mean)*m_MeanDeviation;
  else 
    m_MeanTolerance = m_Mean*m_MeanPercentError;
  m_VarTolerance = m_Var*m_VarPercentError;
}

template <class TInputImage, class TOutputImage>
void
VoronoiSegmentationImageFilter <TInputImage,TOutputImage>::
DrawDiagram(VDImagePointer result,unsigned char incolor,
    unsigned char outcolor,unsigned char boundcolor)
{

  RegionType region = m_InputImage->GetRequestedRegion();
  itk::ImageRegionIteratorWithIndex <VDImage> vdit(result, region);
  while( !vdit.IsAtEnd())
    {    
    vdit.Set(0);
    ++vdit;
    }
  
  EdgeIterator eit;
  EdgeIterator eitend =	m_WorkingVD->EdgeEnd();  
  PointType adds;
  Point<int,2> seeds;

  for(eit = m_WorkingVD->EdgeBegin();eit != eitend; ++eit){
	seeds = m_WorkingVD->GetSeedsIDAroundEdge(eit);
	if((m_Label[seeds[0]]==2)||(m_Label[seeds[1]]==2)){
      drawVDline(result,eit->m_left,eit->m_right,boundcolor);
    }
    else if(m_Label[seeds[0]]){
      drawVDline(result,eit->m_left,eit->m_right,incolor);
    }
    else {
      drawVDline(result,eit->m_left,eit->m_right,outcolor);
    }
  }

}

template <class TInputImage, class TOutputImage>
void
VoronoiSegmentationImageFilter <TInputImage,TOutputImage>::
drawVDline(VDImagePointer result,PointType p1,PointType p2, unsigned char color)
{
  int x1=(int)(p1[0]+0.5);
  int x2=(int)(p2[0]+0.5);
  int y1=(int)(p1[1]+0.5);
  int y2=(int)(p2[1]+0.5);
  if(x1==m_Size[0]) x1--;  
  if(x2==m_Size[0]) x2--;  
  if(y1==m_Size[1]) y1--;  
  if(y2==m_Size[1]) y2--;  

  int dx=x1-x2;
  int adx=(dx>0)?dx:-dx;
  int dy=y1-y2;
  int ady=(dy>0)?dy:-dy;
  int save;
  float curr;
  IndexType idx;
  if (adx > ady){
    if(x1>x2){
	  save=x1; x1=x2; x2=save;
	  save=y1; y1=y2; y2=save;
	}
    curr=(float)y1;
    float offset=(float)dy/dx;
	for(int i=x1;i<=x2;i++){
	  idx[0]=i;
	  idx[1]=y1;
	  result->SetPixel(idx,color);
	  curr += offset;
  	  y1=(int)(curr+0.5);
    }
  }
  else { 
    if(y1>y2){
	  save=x1; x1=x2; x2=save;
	  save=y1; y1=y2; y2=save;
	}
    curr=(float)x1;
    float offset=(float)dx/dy;
	for(int i=y1;i<=y2;i++){
	  idx[0]=x1;
	  idx[1]=i;
	  result->SetPixel(idx,color);
	  curr += offset;
	  x1=(int)(curr+0.5);
    }
  }
}

template <class TInputImage, class TOutputImage>
void
VoronoiSegmentationImageFilter <TInputImage,TOutputImage>::
MakeSegmentBoundary(void)
{

  RegionType region = m_InputImage->GetRequestedRegion(); 
  itk::ImageRegionIteratorWithIndex <OutputImageType> oit(m_OutputImage, region); 
  while( !oit.IsAtEnd())
    {     
    oit.Set(0); 
    ++oit; 
    }

  NeighborIdIterator nit;
  NeighborIdIterator nitend;
  for(int i=0;i<m_NumberOfSeeds;i++){
    if(m_Label[i] == 2){
      nitend = m_WorkingVD->NeighborIdsEnd(i);
	    for(nit=m_WorkingVD->NeighborIdsBegin(i);nit!=nitend;++nit){
	      if(((*nit)>i)&&(m_Label[*nit]==2)){
		      drawLine(m_WorkingVD->getSeed(i),m_WorkingVD->getSeed(*nit));
	      }
	    }
    }
  }
}

template <class TInputImage, class TOutputImage>
void
VoronoiSegmentationImageFilter <TInputImage,TOutputImage>::
Reset(void)
{
  m_WorkingVD->SetRandomSeeds(m_NumberOfSeeds);
  m_StepsRuned = 0;
  m_LastStepSeeds=m_NumberOfSeeds;
  m_NumberOfSeedsToAdded=0;
}

template <class TInputImage, class TOutputImage>
void
VoronoiSegmentationImageFilter <TInputImage,TOutputImage>::
MakeSegmentObject(void)
{
  RegionType region = m_InputImage->GetRequestedRegion(); 
  itk::ImageRegionIteratorWithIndex <OutputImageType> oit(m_OutputImage, region); 
  while( !oit.IsAtEnd())
    {     
    oit.Set(0); 
    ++oit; 
    }
  CellPointer currCell; 
  PointIdIterator currPit;
  PointIdIterator currPitEnd;
  PointType currP;
  PointTypeDeque VertList;
  for(int i=0;i<m_NumberOfSeeds;i++){
    if(m_Label[i] == 1){
      currCell = m_WorkingVD->GetCellId(i);
      currPitEnd = currCell->PointIdsEnd();
	  VertList.clear();
	  for(currPit=currCell->PointIdsBegin();currPit!=currPitEnd;++currPit){
	    m_WorkingVD->GetPointId((*currPit),&(currP));
	    VertList.push_back(currP);
	  }
	  FillPolygon(VertList);
    }
  }
}

template <class TInputImage, class TOutputImage>
void
VoronoiSegmentationImageFilter <TInputImage,TOutputImage>::
FillPolygon(PointTypeDeque vertlist)
{
  IndexType idx;

  PointType currP;
  PointType leftP;
  PointType rightP;
  currP = vertlist.front();
  vertlist.pop_front();
  leftP = vertlist.front();
  while(currP[1] > leftP[1]){
    vertlist.push_back(currP);
    currP=vertlist.front();
    vertlist.pop_front();
    leftP=vertlist.front();
  }
  rightP=vertlist.back();
  while(currP[1] > rightP[1]){
    vertlist.push_front(currP);
    currP=vertlist.back();
    vertlist.pop_back();
    rightP=vertlist.back();
  }
  leftP=vertlist.front();
  PointTypeDeque tmpQ; 
  tmpQ.clear();
  if(leftP[0]>rightP[0]){
    while(!(vertlist.empty())){
      tmpQ.push_back(vertlist.front());
      vertlist.pop_front(); 
    }
    while(!(tmpQ.empty())){
      vertlist.push_front(tmpQ.front());
      tmpQ.pop_front();
    }
  } 
  tmpQ.clear();
  leftP=vertlist.front();
  rightP=vertlist.back();

  double beginy=currP[1];
  int intbeginy=(int)ceil(beginy); 
  idx[1]=intbeginy;
  double leftendy=leftP[1];
  double rightendy=rightP[1];
  double beginx=currP[0];
  double endx=currP[0];
  double leftDx,rightDx;
  double offset;
  double leftheadx=beginx;
  double rightheadx=endx;
  double leftheady=beginy;
  double rightheady=beginy;

  double endy;
  bool RorL;
  int i,j;
  if(leftendy>rightendy){
    RorL=1;
    endy=rightendy;
  }
  else{
    RorL=0;
    endy=leftendy;
  }
  leftDx=(leftP[0]-beginx)/(leftP[1]-beginy);
  rightDx=(rightP[0]-endx)/(rightP[1]-beginy);
  int intendy=(int)floor(endy);
  if(intbeginy>intendy){ //no scanline
    if(RorL){
      endx=rightP[0];
      beginx+=leftDx*(rightP[1]-beginy);
      beginy=rightP[1];
    }
    else{
      beginx=leftP[0];
      endx+=rightDx*(leftP[1]-beginy); 
      beginy=leftP[1];
    }
  }
  else if((intbeginy==intendy) && (intbeginy==0)){ //only one scanline at 0;
    if(RorL) endx=rightP[0];
    else beginx=leftP[0];
    for(i=ceil(beginx);i<=floor(endx);i++){
      idx[0]=i;
      m_OutputImage->SetPixel(idx,1);  
    }
    idx[1]=idx[1]+1;
  }
  else{ //normal case some scanlines
    offset=(double)intbeginy-beginy;
    endx+=offset*rightDx;
    beginx+=offset*leftDx;
    while(idx[1]<=intendy){
      for(i=ceil(beginx);i<=floor(endx);i++){
        idx[0]=i;
        m_OutputImage->SetPixel(idx,1);  
      }
      endx+=rightDx;
      beginx+=leftDx;        
      idx[1]=idx[1]+1;
    }
    beginy=endy;
  }

  int vsize=vertlist.size();
  while(vsize>2){
    vsize--;
    if(RorL){
      vertlist.pop_back();
      currP=rightP;
      rightheadx=currP[0];
      rightheady=currP[1];
      endx=currP[0];
      beginx=leftheadx+leftDx*(beginy-leftheady); 
      rightP=vertlist.back();
      rightDx=(rightP[0]-currP[0])/(rightP[1]-currP[1]);
    }
    else{
      vertlist.pop_front();
      currP=leftP;
      leftheadx=currP[0];
      leftheady=currP[1];
      beginx=currP[0];
      endx=rightheadx+rightDx*(beginy-rightheady);
      leftP=vertlist.front();
      leftDx=(leftP[0]-currP[0])/(leftP[1]-currP[1]);
    }
        
    leftendy=leftP[1];
    rightendy=rightP[1];
    if(leftendy>rightendy){
      RorL=1;
      endy=rightendy;
    }
    else{
      RorL=0;
      endy=leftendy;
    }

    intendy=(int)floor(endy);
    intbeginy=(int)ceil(beginy); 

    if(intbeginy>intendy){ //no scanline
      if(RorL){
        endx=rightP[0];
        beginx+=leftDx*(rightP[1]-beginy);
        beginy=rightP[1];
      }
      else{
        beginx=leftP[0];
        endx+=rightDx*(leftP[1]-beginy); 
        beginy=leftP[1];
      }
    }
    else{ //normal case some scanlines
      offset=(double)intbeginy-beginy;
      endx+=offset*rightDx;
      beginx+=offset*leftDx;
      while(idx[1]<=intendy){
        for(i=ceil(beginx);i<=floor(endx);i++){
          idx[0]=i;
          m_OutputImage->SetPixel(idx,1);  
        }
        endx+=rightDx;
        beginx+=leftDx;        
        idx[1]=idx[1]+1;
      }
      beginy=idx[1];
    }
  }


  if(RorL){
    beginy=rightP[1];
    endy=leftP[1];
  }
  else{
    beginy=leftP[1];
    endy=rightP[1];
  }
  intbeginy=(int)ceil(beginy);
  intendy=(int)floor(endy);
  if(intbeginy<=intendy){
    if(RorL){
      rightDx=(rightP[0]-leftP[0])/(rightP[1]-leftP[1]);
      endx=rightP[0];
      beginx=leftP[0]+leftDx*(rightP[1]-leftP[1]);
    }
    else{
      leftDx=(rightP[0]-leftP[0])/(rightP[1]-leftP[1]);
      beginx=leftP[0];
      endx=rightP[0]+rightDx*(leftP[1]-rightP[1]);
    }
    offset=(double)intbeginy-beginy;
    beginx+=offset*leftDx;
    endx+=offset*rightDx;
    while(idx[1]<=intendy){
      for(i=ceil(beginx);i<=floor(endx);i++){
        idx[0]=i;
        m_OutputImage->SetPixel(idx,1);  
      }
      endx+=rightDx;
      beginx+=leftDx;        
      idx[1]=idx[1]+1;
    }
  }

}



} //end namespace

#endif

