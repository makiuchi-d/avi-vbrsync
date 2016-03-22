#include <stdio.h>
#include <windows.h>
#include <avifmt.h>
#include <aviriff.h>
#include <mmreg.h>


typedef struct {
	AVIStreamHeader vidsh,audsh;
	MPEGLAYER3WAVEFORMAT wf;
	size_t vidsh_pos,audsh_pos,wf_pos;
	size_t vidsh_size,audsh_size,wf_size;
} STREAMHEADERS;

int read_headers(FILE *fp,STREAMHEADERS *hs);

/*****************************************************************************/
int main(int argc,char *argv[])
{
	int il;
	FILE *avifile;
	STREAMHEADERS hs;

	if(argc<2){
		fputs("Usage: vbrsync <AVIFileName>",stderr);
		return 0;
	}

	printf("Interleave interval : ");
	scanf("%d",&il);

	if((avifile = fopen(argv[1],"r+b"))==NULL){
		fputs("File open error.",stderr);
		return 0;
	}
	fseek(avifile,0,SEEK_SET);

	if(!read_headers(avifile,&hs)){
		goto EXIT;
	}

	printf("frame rate   : %d/%d\n",hs.vidsh.dwRate,hs.vidsh.dwScale);
	printf("video length : %d\n",hs.vidsh.dwLength);
	printf("audio rate   : %d/%d -> %d/%d\n",
		   hs.audsh.dwRate,hs.audsh.dwScale,hs.vidsh.dwRate,hs.vidsh.dwScale*il);
	printf("audio length : %d -> %d\n",hs.audsh.dwLength,hs.vidsh.dwLength/il);
	printf("block align  : %d -> %d\n",hs.wf.wfx.nBlockAlign,hs.audsh.dwSuggestedBufferSize);

	hs.audsh.dwRate = hs.vidsh.dwRate;
	hs.audsh.dwScale = hs.vidsh.dwScale * il;
	hs.audsh.dwLength = hs.vidsh.dwLength / il;
	hs.wf.wfx.nBlockAlign = hs.audsh.dwSuggestedBufferSize;

	fseek(avifile,hs.vidsh_pos,SEEK_SET);
	fwrite(&hs.vidsh,hs.vidsh_size,1,avifile);
	fseek(avifile,hs.audsh_pos,SEEK_SET);
	fwrite(&hs.audsh,hs.audsh_size,1,avifile);
	fseek(avifile,hs.wf_pos,SEEK_SET);
	fwrite(&hs.wf,hs.wf_size,1,avifile);

EXIT:
	fclose(avifile);
	return 0;
}



/*===========================================================================*/
int read_headers(FILE *fp,STREAMHEADERS *hs)
{
	RIFFCHUNK ch;
	RIFFLIST list;
	size_t pos;

	fread(&list,sizeof(list),1,fp);
	if(list.fcc!=FCC('RIFF') || list.fccListType!=FCC('AVI ')){
		fputs("Not a AVI file.",stderr);
		return 0;
	}
	fread(&list,sizeof(list),1,fp);
	if(list.fcc!=FCC('LIST')||list.fccListType!=FCC('hdrl')){
		fputs("'hdrl' list is not found.",stderr);
		return 0;
	}
	fread(&ch,sizeof(ch),1,fp);
	if(ch.fcc!=FCC('avih')){
		fputs("'avih' chunk is not found.",stderr);
		return 0;
	}
	fseek(fp,ch.cb,SEEK_CUR);
	fread(&list,sizeof(list),1,fp);
	if(list.fcc!=FCC('LIST') || list.fccListType!=FCC('strl')){
		fputs("'strl'(0) list is not found.",stderr);
		return 0;
	}
	pos = ftell(fp) + list.cb - 4;
	fread(&ch,sizeof(ch),1,fp);
	if(ch.fcc!=FCC('strh')){
		fputs("'strh'(0) chunk is not found.",stderr);
		return 0;
	}
	hs->vidsh_pos = ftell(fp);
	hs->vidsh_size = ch.cb;
	fread(&(hs->vidsh),ch.cb,1,fp);
	if(hs->vidsh.fccType!=FCC('vids')){
		fputs("'vids' header is not found.",stderr);
		return 0;
	}
	fseek(fp,pos,SEEK_SET);
	fread(&list,sizeof(list),1,fp);
	if(list.fcc!=FCC('LIST') || list.fccListType!=FCC('strl')){
		fputs("'strl'(1) list is not found.",stderr);
		return 0;
	}
	fread(&ch,sizeof(ch),1,fp);
	if(ch.fcc!=FCC('strh')){
		fputs("'strh'(1) chunk is not found.",stderr);
		return 0;
	}
	hs->audsh_pos = ftell(fp);
	hs->audsh_size = ch.cb;
	fread(&(hs->audsh),ch.cb,1,fp);
	if(hs->audsh.fccType!=FCC('auds')){
		fputs("'auds' header is not found.",stderr);
		return 0;
	}
	fread(&ch,sizeof(ch),1,fp);
	if(ch.fcc!=FCC('strf')){
		fputs("'strf'(1) chunk is not found.",stderr);
		return 0;
	}
	hs->wf_pos = ftell(fp);
	hs->wf_size = ch.cb;
	fread(&hs->wf,min(sizeof(MPEGLAYER3WAVEFORMAT),ch.cb),1,fp);

	return 1;
}


