/*******************************************************************************
                                       *
*                                                                              *
*******************************************************************************/

#include "Viewer.h"

int main(int argc, char** argv)
{
	openni::Status rc = openni::STATUS_OK;

	SampleViewer sampleViewer("Hand Point Viewer");

	rc = sampleViewer.Init(argc, argv);
	if (rc != openni::STATUS_OK)
	{
		return 1;
	}
	sampleViewer.Run();
}
