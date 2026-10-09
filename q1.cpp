#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <termios.h>
#include <readline/readline.h>
#include <readline/history.h>
using namespace std;
string shell_home = "";
string prev_dir="";

pid_t fpid=-1;

vector<string> cmds = {"cd","ls","echo","pwd","pinfo","history","search","exit"};

string display_user(){


    char* username=getlogin();
    if(!username) username=(char*)"user";

    char host[516];
    gethostname(host,sizeof(host));

    char curr_directory[1024];
    getcwd(curr_directory,sizeof(curr_directory));

    string path=curr_directory;
    const string &home=shell_home.empty()? string(getenv("HOME")?getenv("HOME"):""):shell_home;;
        if(!home.empty() && path.find(home)==0){
            path.replace(0,home.length(),"~");

        }


    return string(username) + "@" + host + ":" + path + ">";

}

string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\n");
    size_t end = s.find_last_not_of(" \t\n");
    if(start == string::npos) return "";
    return s.substr(start, end - start + 1);
}


vector<string>cmdsplit(const string &cmd){

    vector<string> res;
    string s;
    for(char c: cmd){
        if(c==';'){
            if(!s.empty()){
                res.push_back(s);
                s.clear();
            }
        }
        else s.push_back(c);
    }
    if(!s.empty()) res.push_back(s);
    return res;

}
void tokanize(string &command,vector<string> &tokens){
    string t;
    istringstream s(command);
    while (s >> t){
        tokens.push_back(t);

    }
}

void cdFunction(vector<string> &tokens){
    int n=tokens.size();
    if(n>2){
        perror("Invalid command");
    }
    char curr_directory[1024];
    getcwd(curr_directory,sizeof(curr_directory));
   
    string new_dir;
    if(n==1 ||tokens[1]=="~"){
        new_dir=getenv("HOME");
    }
    else if(tokens[1]=="-"){
        if(prev_dir.empty()){
            perror("no previous directory exist");
        }
        new_dir=prev_dir;
        

    }
    else{
        new_dir=tokens[1];
    }
    if(chdir(new_dir.c_str()) != 0){
        perror("no such directory or file exist");

    }
    else prev_dir=curr_directory;




    

}

void echoFunction(vector<string> &tokens){
    for(size_t i=1;i<tokens.size();i++){
        cout<<tokens[i]<<" ";
    }
    cout<<endl;
    
}

void pwdFunction(){
    char curr_directory[1024];
    if(getcwd(curr_directory,sizeof(curr_directory))!=NULL){
        cout<<curr_directory<<endl;
    }
    else{
        perror("current directory path fatching failed");
    }
}

void permissions(mode_t mode,string &p){
    if (S_ISREG(mode)) p += "-";
    else if (S_ISDIR(mode)) p += "d";
    else if (S_ISLNK(mode)) p += "l";
    else p += "unknown";

    if(mode & S_IRUSR) p+="r";
    else p+="-";
    if(mode & S_IWUSR) p+="w";
    else p+="-";
    if(mode & S_IXUSR) p+="x";
    else p+="-";
    if(mode & S_IRGRP) p+="r";
    else p+="-";
    if(mode & S_IWGRP) p+="w";
    else p+="-";
    if(mode & S_IXGRP) p+="x";
    else p+="-";
    if(mode & S_IROTH) p+="r";
    else p+="-";
    if(mode & S_IWOTH) p+="w";
    else p+="-";
    if(mode & S_IXOTH) p+="x";
    else p+="-";

}
void lsFunction(vector<string> &tokens){
    vector<string>path;
    bool fa=false;
    bool fl=false;
    bool f=false;
    for(size_t i=1;i<tokens.size();i++){
        if(tokens[i][0]=='-'){
            for(char c:tokens[i]){
                if(c=='a') fa=true;
                if(c=='l') fl=true;
            }
        }
        else if(tokens[i][0]=='~') f=true;
            else{
                path.push_back(tokens[i]);
            }
        
    }
    if(path.empty()) path.push_back(".");

    for(size_t i=0;i<path.size();i++){
        string p=path[i];
        struct stat pstat;
        if(stat(p.c_str(),&pstat)==-1){
            perror("NO such path exist");
            continue;
        }
        
        
        if (S_ISREG(pstat.st_mode)){
            if(fl){
                string perm;
                permissions(pstat.st_mode,perm);
                char time[1024];
                strftime(time, sizeof(time), "%b %d %H:%M", localtime(&pstat.st_mtime));

                struct passwd *userinfo=getpwuid(pstat.st_uid);
                struct group *groupinfo=getgrgid(pstat.st_gid);

                cout<<perm<<" "<<pstat.st_nlink<<" "
                    <<(userinfo?userinfo->pw_name:"unknown")<<" "
                    <<(groupinfo?groupinfo->gr_name:"unknown")<<" "
                    <<setw(8) << pstat.st_size << " "
                    <<time<<" "
                    <<p<<endl;
            }
            else if(f) cout<<p<<endl;
            else cout<<p<<endl;
            continue;

        }
        DIR *dir=opendir(p.c_str());
        if(!dir){
            perror("can not open directory");
            continue;
        }
        if(path.size()>1){
            cout<<p<<":"<<endl;
        }

        struct dirent *dirdetails;
        while((dirdetails = readdir(dir)) != NULL){
            string dirname=dirdetails->d_name;
            if(!fa && dirname[0]=='.') continue;
            if(dirname=="." || dirname=="..") continue;

            string finalpath;
            finalpath=p+"/"+dirname;
            struct stat newst;
            if(stat(finalpath.c_str(),&newst)==-1){
                perror("can not access finalpath");
                continue;
            }
        

            if(fl){
                string perm;
                permissions(newst.st_mode,perm);
                char time[1024];
                strftime(time, sizeof(time), "%b %d %H:%M", localtime(&newst.st_mtime));

                struct passwd *userinfo=getpwuid(newst.st_uid);
                struct group *groupinfo=getgrgid(newst.st_gid);

                cout<<perm<<" "<<newst.st_nlink<<" "
                    <<(userinfo?userinfo->pw_name:"unknown")<<" "
                    <<(groupinfo?groupinfo->gr_name:"unknown")<<" "
                    <<setw(8) << newst.st_size << " "
                    <<time<<" "
                    <<dirname<<endl;
            }
            else if(f) cout<<dirname<<endl;
            else{
                cout<<dirname<<endl;
            }
        }

        closedir(dir);
        if(i<path.size()-1) cout<<endl;

        
    }
    
    

}

void backforcmd(vector<string> &tokens){
    bool flag=false;
    if( tokens.size()>0 && tokens.at(tokens.size()-1)=="&"){
        flag=true;
        tokens.pop_back();

    }
    pid_t id=fork();
    if(id<0){
        perror("fork command does not work");
        return;
    }
    if(id==0){
        char* v[tokens.size()+1];
        for(size_t i=0;i<tokens.size();i++){
            v[i]=(char*)tokens[i].c_str();
        }
        v[tokens.size()]=NULL;
        execvp(v[0],v);
        cerr<<"could not create process\n";
        exit(1);
    }
    else{
    if(!flag){
        fpid=id;
        waitpid(id,NULL,WUNTRACED);
        fpid=-1;
    }
    else cout<<"["<<id<<"]"<<"running in background"<<endl;
   }
    

    

}


void pinfoFunction(vector<string> &tokens){
    int id;
    int n=tokens.size();
    if(n==1) id=getpid();
    else if(n==2) id=atoi(tokens[1].c_str());
    else {
        cout<<"wrong command"<<endl;
        exit(1);
    }

    string file="/proc/" + to_string(id) + "/stat";
    int fdisc=open(file.c_str(),O_RDONLY);

    if(fdisc<0){
        cerr<<"process with this id is not present\n ";
        return;

    }

    char b[1024];
    int n1=read(fdisc,b,sizeof(b)-1);
    b[n1]='\0';
    if(n1<=0){
        cerr<<"not able to read the file\n";
        return;
    }

    char status='\0';
    long long memory;
    string executablepath="/proc/" + to_string(id) + "/exe";
    int c=0;
    int p=-1;
    int t=-1;
    for(int i=0;b[i]!='\0';i++){
        if(b[i]==' ') c++;
        if(c==2 && status=='\0') status=b[i+1];
        if(c==4 && p==-1){
            p=strtol(&b[i+1],NULL,10);
        }
        if(c==7 && t==-1){
            t=strtol(&b[i+1],NULL,10);
        }
        if(c==22) memory=strtoul(&b[i+1],NULL,10);
    }
    string newstatus=string(1,status);
    if(t!=-1 && p!=-1 && t==p){
        newstatus+="+";
    }

    char exepath[1024];
    int l=readlink(executablepath.c_str(),exepath,sizeof(exepath)-1);
    if(l!=-1) exepath[l]='\0';
    else cerr<<"executablepath not found\n";

    cout<<"process id : "<<id<<endl;
    cout<<"process status : "<<newstatus<<endl;
    cout<<"memory : "<<memory<<endl;
    cout<<"Executable Path : "<<exepath<<endl;



    
    
}

bool searchfile(string &currdir,string &filename){
    DIR *dir=opendir(currdir.c_str());
    struct dirent *dirinfo;
    
        if(!dir){
            return false;
        }
        
        while((dirinfo=readdir(dir))!=NULL){
            string dirname=dirinfo->d_name;
            if(dirname=="." || dirname=="..") continue;
            string fullpath=currdir +"/" + dirname;

            if(dirname==filename){
                return true;
            }
            struct stat newst;
            if(stat(fullpath.c_str(),&newst)==0 && S_ISDIR(newst.st_mode)){
                if(searchfile(fullpath,filename)){
                    closedir(dir);
                    return true;
                }
            }
            
        }

    closedir(dir);
    return false;

}
void inputred(string &inputfile){
    int inputfd=open(inputfile.c_str(),O_RDONLY);
    if(inputfd<0){
        perror("not able to open file");
        exit(1);
    }
    dup2(inputfd,STDIN_FILENO);
    close(inputfd);

}

void iored(vector<string> &tokens){
    string inputfile,outputfile;
    bool update=false;
    int n=tokens.size();
    int i=0;
    vector<char*> v;

    while(i<n){
        if(tokens[i]=="<") inputfile=tokens[++i];
        else if(tokens[i]==">") {
            outputfile=tokens[++i];
        }
        else if(tokens[i]==">>"){
            outputfile=tokens[++i];
            update=true;
        }
        else v.push_back((char*)tokens[i].c_str());
        i++;
    }
    v.push_back(NULL);

    if(fork()==0){
        if(!inputfile.empty()) inputred(inputfile);
        if(!outputfile.empty()){
            int outputfd;
            if(update){
            outputfd=open(outputfile.c_str(),O_WRONLY | O_CREAT | O_APPEND,0644);
            }
            else outputfd=open(outputfile.c_str(),O_WRONLY | O_CREAT | O_TRUNC,0644);

            if(outputfd<0){
                perror("error in output file");
                exit(1);

            }
            dup2(outputfd,STDOUT_FILENO);
            close(outputfd);
        }

        execvp(v[0],v.data());
        perror("execvp command failed");
        exit(1);
        

    }
    else wait(NULL);
    fflush(stdout);


}

vector<vector<string>> splitcmd(vector<string> & tokens,vector<vector<string>> &cmd){
    vector<string> s;
    for(size_t i=0;i<tokens.size();i++){
        if(tokens[i]=="|"){
            cmd.push_back(s);
            s.clear();
        }
        else s.push_back(tokens[i]);
    }
    cmd.push_back(s);
    return cmd;

}
void checkred(vector<string> &cmd,string &inputfile,string &outputfile,bool &update,vector<char*> &v){
    for(size_t i=0;i<cmd.size();i++){
        if(cmd[i]=="<") inputfile=cmd[++i];
        else if(cmd[i]==">"){
            outputfile=cmd[++i];
        }
        else if(cmd[i]==">>"){
            outputfile=cmd[++i];
            update=true;
        }
        else v.push_back((char*)cmd[i].c_str());
    }
    v.push_back(NULL);
     
}

void pipeline(vector<string> &tokens){
    vector<vector<string>>cmd;
    cmd=splitcmd(tokens,cmd);
    int n=cmd.size();
    int prfd=-1;
    
    vector<pid_t>vpid;
    
    for(int i=0;i<n;i++){
        int pfd[2];
        if(i<n-1) pipe(pfd);
        pid_t pid = fork();

        if(pid==0){
            if(i<n-1){
                dup2(pfd[1],STDOUT_FILENO);
                close(pfd[0]);
                close(pfd[1]);
            }

            if(prfd!=-1){
                dup2(prfd,STDIN_FILENO);
                close(prfd);
            }
            string inputfile,outputfile;
            bool update=false;
            vector<char*> v;
            if (i == 0 || i == n-1) {
                checkred(cmd[i], inputfile, outputfile, update, v);
            } else {
    
                    for (auto &c : cmd[i]) {
                    v.push_back((char*)c.c_str());
                    }   
                    v.push_back(NULL);
            }

            if(i==0 && !inputfile.empty()) inputred(inputfile);
            if(i==n-1 && !outputfile.empty()){
                int outputfd;
                if(update){
                    outputfd=open(outputfile.c_str(),O_WRONLY | O_CREAT | O_APPEND,0644);
                }
                else outputfd=open(outputfile.c_str(),O_WRONLY | O_CREAT | O_TRUNC,0644);

                if(outputfd<0){
                    perror("error in output file");
                    exit(1);

                }
                dup2(outputfd,STDOUT_FILENO);
                close(outputfd);
            }

            execvp(v[0],v.data());
            perror("execvp command failed");
            exit(1);

        }
        else{
            vpid.push_back(pid);
            if(prfd!=-1){
                close(prfd);
            }
            if(i<n-1){
                close(pfd[1]);
                prfd=pfd[0];
            }
        }
    }
    for (pid_t cpid : vpid) {
        waitpid(cpid, NULL, 0);
    }

}

void controlc(int sig){
    (void)sig; 

    if (fpid > 0) {
       
        kill(fpid, SIGINT);
        fpid = -1; 
    } else {
        
        const char msg[] = "\n^C\n";
        write(STDOUT_FILENO, msg, sizeof(msg)-1);
        rl_replace_line("", 0); 
        rl_on_new_line();       
         
        rl_redisplay(); 
    }
}

void controlz(int sig){
    (void)sig; 

    if (fpid > 0) {
       
        kill(fpid, SIGTSTP);
     
        fpid = -1; 
    }
    else {
        
      
        rl_replace_line("", 0); 
        rl_on_new_line();       
         
        rl_redisplay(); 
    }


}

void controld(int sig){
    (void)sig;
    cout<<"exiting shell"<<endl;
    exit(1);
}

char* cmdgen(const char* t,int s){
    size_t i;
    vector<string> m;
    if(s==0){
        i=0;
        m.clear();
        string pre(t);
        if(pre.empty()) return NULL;
        for(auto &cd : cmds){
            if(cd.rfind(pre,0)==0){
                m.push_back(cd);
            }
        }
    }
    if (i < m.size())
        return strdup(m[i++].c_str());
    return NULL;

}

char** autocomplete(const char* t,int s,int e){
    (void)e;
    if(s==0) return rl_completion_matches(t, cmdgen);
    else return rl_completion_matches(t, rl_filename_completion_function);
}

int main(){

    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    // term.c_lflag &= ~ECHOCTL;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);

    signal(SIGINT, controlc);    
    signal(SIGTSTP, controlz); 
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
     
    char launchcd[1024];
    if(getcwd(launchcd,sizeof(launchcd))!=NULL){
        shell_home=string(launchcd);
    }

    string historyfile=string(getenv("HOME")) + "/" + ".myshell_history";
    read_history(historyfile.c_str());

    

    rl_attempted_completion_function=autocomplete;
    rl_bind_key('\t', rl_complete);

    while(true){
        string s=display_user();

        char* input=nullptr;
        do {
            input = readline(s.c_str());
        } while (input == nullptr && errno == EINTR);
        if(!input) break;

        string command(input);
        free(input);

        if(command=="exit") break;
        if(command.empty()) continue;
        

        add_history(command.c_str());
        write_history(historyfile.c_str());

        vector<string> allcmds=cmdsplit(command);   

        for(auto &cd: allcmds){
            cd = trim(cd);
            if(cd.empty()) continue;

            vector<string>tokens;
            tokanize(cd,tokens);
            if(tokens.empty()) continue;
            

            if(tokens[0]=="history"){
                HIST_ENTRY **list=history_list();
                int count=10;
                if(tokens.size()==2){
                    count=stoi(tokens[1]);
                    if(count>20) count =20;
                }
                
                if(list){
                    int t=history_length;
                    int s=max(0,t-count);
                    for(int i=s;i<t;i++){
                    cout<<list[i]->line<<endl;
                    }
                }
                continue;
            }
        

        bool pipef = false;
        for (size_t i=0;i<tokens.size();i++) {
            if (tokens[i] == "|") {
            pipef = true;   
            break;
            }
        }
        if(pipef){
            pipeline(tokens);
            continue;
        }

         if (tokens[0] == "cd") {
            cdFunction(tokens);
        } else if (tokens[0] == "echo") {
            bool ioredf=false;
            for(auto &tk:tokens){
                if (tk == "<" || tk == ">" || tk== ">>") {
                    ioredf = true;
                    break;
                }
            }
            if(ioredf) iored(tokens);
           else echoFunction(tokens);
        } else if (tokens[0] == "pwd") {
            pwdFunction();
        } else if(tokens[0]=="ls"){
            lsFunction(tokens);
        } else if(tokens[0]=="pinfo"){
            pinfoFunction(tokens);
        }
        else if(tokens[0]=="search"){
            string currdir=".";
            if(searchfile(currdir,tokens[1])) cout<<"true"<<endl;
            else cout<<"false"<<endl;
            
        }
         else {
            bool ioredf=false;
            for(auto &tk:tokens){
                if (tk == "<" || tk == ">" || tk== ">>") {
                    ioredf = true;
                    break;
                }
            }
            if(ioredf) iored(tokens);
           else backforcmd(tokens);
        }
        }
    }
}