//TODO: does reference implementation perform sanity checks?

#include "response_system.h"
#include "map_bsp.hpp"
#include <filesystem>
#include <vector>


#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <iterator>


std::vector<std::string> RRParser::entryPointsToParse = {"scripts/talker/response_rules.txt"} ;//path hardcoded. Why vector? we might have multiple files to parse;
std::vector<RRParser::CResponseRulesScript> RRParser::responseSystems;
void RRParser::initRules(std::string gameDir)
{
    //std::filesystem::create_directory(gameDir+"tmp_rr"); //tmp directory for script parsing.

    for (auto entryPoint = entryPointsToParse.begin();entryPoint != entryPointsToParse.end();++entryPoint) {
        std::string absoluteFile = gameDir+"/";
        absoluteFile += *entryPoint;
        std::ifstream file(absoluteFile.c_str());
        CResponseRulesScript script;
        if (std::strcmp(entryPoint->c_str(),"scripts/talker/response_rules.txt")==0)
            script.isGlobal=true;
        SPDLOG_INFO("Currently parsing: {0}", *entryPoint);
        script.parseScript(gameDir,file);
        responseSystems.push_back(script);
        SPDLOG_INFO("Succesfully parsed {0}", *entryPoint);
        file.close();
    }


    //Valve resolves includes by recursively reading files, see CResponseSystem::ParseInclude for details
    //This method has also implicit include guards


}
void RRParser::dumpSceneNames()
{
    std::vector<BSPParser::Map_Scene> scenes;
    scenes.reserve(65535);
    for (const auto& rrSys: responseSystems) {
        for (const auto& rGroup: rrSys.responseGroups) {
            for (const auto& response: rGroup.responses) {
                if (response.type == RRParser::EResponseType::SCENE) {
                    std::string responseVCD = response.typeParam;
                    if (responseVCD.find("$gender")!=std::string::npos) {
                        std::string maleVCD,femaleVCD;
                        maleVCD = femaleVCD = responseVCD;
                        Helper::ReplaceAll(maleVCD,"$gender","male");
                        Helper::ReplaceAll(femaleVCD,"$gender","female");
                        BSPParser::Map_Scene maleScene(maleVCD.c_str()),femaleScene(femaleVCD.c_str());
                        scenes.emplace_back(maleScene);
                        scenes.emplace_back(femaleScene);
                    } else {


                        BSPParser::Map_Scene scene(responseVCD.c_str());
                        scenes.emplace_back(scene);
                    }
                }
            }
        }
    }
    scenes.shrink_to_fit();
    SPDLOG_INFO("Found {0} scene file names from Response System!", scenes.size());
    BSPParser::Scenes.emplace("responseSystem",scenes);

}




void RRParser::CResponseRulesScript::parseScript(std::string gamedir,std::ifstream &file)
{
    using namespace boost; //for tokenizer
    for (std::string line;safeGetline(file,line);) {
        //nuke the comments
        line = line.substr(0,line.find("//"));
        char_separator<char> sep(" \t");

        tokenizer<char_separator<char>> tokens(line,sep);
        auto tok_it= tokens.begin();

        if(tok_it!=tokens.end()) {
            if (tok_it.current_token().compare("#include")==0) {
                //next token is the included file...
                std::string includedFile = (++tok_it).current_token();
                stripQuotes(includedFile);
                includedFile = gamedir+"scripts/"+includedFile;
                if (std::find(includedFiles.begin(),includedFiles.end(),includedFile)==includedFiles.end()) {

                    std::ifstream includedStream(includedFile);
                    parseScript(gamedir,includedStream);
                    includedFiles.push_back(includedFile);
                    includedStream.close();
                }
                //that's the line!

            }
            if (boost::iequals(tok_it.current_token(), "enumeration")) {
                //next token is enum's name.
                std::string enumName = (++tok_it).current_token();
                stripQuotes(enumName);
                CScriptEnumeration addedEnum(enumName);
                addedEnum.parseEnum(file);
                enums.push_back(addedEnum);

                continue; //we're long past tokenized line (getline state is saved to the stream).
            }

            if(boost::iequals(tok_it.current_token(), "response")) {
                // quite a lot of parsing in a single line...
                //back up line tokens just in case
                std::vector<std::string> responseTokens(tokens.begin(),tokens.end());

                // anyways next token is the groupname
                std::string responseGName = (++tok_it).current_token();
                stripQuotes(responseGName);
                bool isOneLiner = false;
                CScriptResponseGroup rGroup(responseGName);

                //one member group is a oneliner, so check if we have pending tokens

                if (std::next(tok_it) != tokens.end()) {
                    isOneLiner = true;
                }
                if (isOneLiner) {
                    //if so parse Response flags before response proper.
                    //prepare flags partition

                    std::vector<std::string> flagsStrings(responseTokens.begin()+2,responseTokens.end()-2);
                    CScriptResponse response;
                    response.parseFlags(flagsStrings);
                    std::vector<std::string> typeStrings(responseTokens.end()-2,responseTokens.end()); // two last tokens.
                    response.parseType(typeStrings);
                    rGroup.responses.push_back(response);
                    continue; //parse new line.
                } else {
                    //we're in end of the line. Check for next Line until we hit { or root token. If root token was found step back one line
                    //TODO: multiple lines and null response handling
                    rGroup.parseResponseGroup(file);
                }
                responseGroups.push_back(rGroup);
            }

            if(boost::iequals(tok_it.current_token(), "criterion")) {
                //that's a one liner too.
                //make vector of tokens then.
                std::vector<std::string> criterionTokens(tokens.begin(),tokens.end());
                // next token is the name;
                std::string name = criterionTokens[1];
                stripQuotes(name);
                CScriptCriterion criterion(name);
                auto criteronFlags(std::vector<std::string>(criterionTokens.begin()+2,criterionTokens.end()));
                criterion.parseCriterion(criteronFlags);
                criteria.push_back(criterion);
                continue;
            }

            if(boost::iequals(tok_it.current_token(), "rule")) {
                std::string ruleName = (++tok_it).current_token();
                CScriptRule rule(ruleName);
                rule.parseRule(file);
                rules.push_back(rule);
            }

            //tok_it++; //TODO: Is this needed?
        }


    }

}

void RRParser::CScriptEnumeration::parseEnum(std::ifstream &file)
{
    using namespace boost; //tokenizer again :)

    for (std::string line; safeGetline(file,line);) {
        line = line.substr(0,line.find("//"));
        char_separator<char> sep(" \t");

        tokenizer<char_separator<char>> tokens(line,sep);
        // we should expect 2 tokens at most


        std::vector<std::string> tokenList(tokens.begin(),tokens.end()); //we should have list of tokens here

        if(tokenList[0]=="{")
            continue; //start of the enum
        if(tokenList[0]=="}")
            break; //end of the enum

        //We're in keyvalues now

        std::string key = tokenList[0],value = tokenList[1];
        stripQuotes(key);stripQuotes(value);
        KV.emplace(key,value);


    }

    return;

}

void RRParser::CScriptResponseGroup::parseResponseGroup(std::ifstream &file)
{
    using namespace boost;
    auto oldPointer = file.tellg();


    bool foundBlock=false;
    for(std::string line; safeGetline(file,line);) {
        line = line.substr(0,line.find("//"));
        char_separator<char> sep(" \t");

        noRepeats = false; // this piece of code is here since default value of noRepeats is true.

        tokenizer<char_separator<char>> tokens(line,sep);
        // we should expect 2 tokens at most
        std::vector<std::string> tokenList(tokens.begin(),tokens.end()); //we should have list of tokens here



        if (tokenList.size()>0) {
            if(!foundBlock && RRParser::CResponseRulesScript::isRootToken(tokenList[0])) {
                file.seekg(oldPointer);
                return; //Force back the pointer and relinquish our control.
            }
            if (tokenList[0]=="{") {
                //that's our block! update the state.
                foundBlock=true;
            }

            if (tokenList[0]=="}") {
                //we are done.
                break;

            }

            //group-wide flags

            if(tokenList[0]=="permitrepeats"){
                permitRepeats=true;
            }

            if (tokenList[0]=="norepeat"){
                noRepeats=true;
            }

            if (tokenList[0]=="sequential") {
                sequential=true;
            }


            if (tokenList.size()>1) {
                CScriptResponse response;
                std::vector<std::string> typeStrings(tokenList.begin(),tokenList.begin()+2);
                std::vector<std::string> flagStrings(tokenList.begin()+2,tokenList.end());
                response.parseType(typeStrings);
                response.parseFlags(flagStrings);
                responses.push_back(response);
            }
            if (!foundBlock){
                oldPointer = file.tellg();//keep searching
            }

        }
        }
}






void RRParser::CScriptResponse::parseFlags(std::vector<std::string> &flags)
{
    for (auto token=flags.begin();token!=flags.end();token++) {
        stripQuotes(*token); //does this change element in place? if so are current iterators still valid?
        //delay
        if (token->compare("nodelay")==0) {
            this->delayStart = this->delayEnd=0;
        } else if (token->compare("defaultdelay")==0) {
            this->delayStart = DEF_MIN_DELAY;this->delayEnd = DEF_MAX_DELAY;
        } else if (token->compare("delay")==0) {
            ++token;// next token is the range.
            std::string range = *token;
            stripQuotes(range);
            std::string rangeBegin = range.substr(0,range.find(','));
            std::string rangeEnd = range.substr(range.find(',')+1);
            this->delayStart = std::stof(rangeBegin);this->delayEnd = std::stof(rangeEnd);

        } else if (token->compare("speakonce")==0) {

            this->speakOnce = true;
        } else if (token->compare("noscene")==0) {
            //TODO: check if scene is asked to bypass scene system. Meaning scene type with noscene flag
            this->bypassScene = true;
        } else if (token->compare("odds")==0) {
            ++token;
            std::string odds = *token;
            stripQuotes(odds);
            this->odds = std::stoi(odds);
        } else if (token->compare("respeakdelay")==0) {
            ++token;// next token is the range.
            std::string range = *token;
            stripQuotes(range);
            std::string rangeBegin = range.substr(0,range.find(','));
            std::string rangeEnd = range.substr(range.find(',')+1);
            this->respeakDelayStart = std::stof(rangeBegin);this->respeakDelayEnd = std::stof(rangeEnd);

        } else if (token->compare("soundlevel")==0) {
            ++token;
            std::string soundlvl = *token;
            stripQuotes(soundlvl);
            this->soundlevel = soundlvl;
        }
        // first check if element is forcibly shoved to head or tail. Those flags ignore weight.
        else if (token->compare("displayfirst")==0) {
            this->displayFirst = true;
       } else if (token->compare("displaylast")==0) {
            this->displayLast = true;
       }
        // weight
        else if (token->compare("weight")==0) {
            ++token;
            std::string weight = *token;
            stripQuotes(weight);
            this->weight = std::stof(weight);
        }
    }

}

void RRParser::CScriptResponse::parseType(std::vector<std::string> &types)
{
    std::string type = types[0],argument = types[1];
    stripQuotes(type);stripQuotes(argument);
    this->typeParam=argument;

    if (type=="speak") {
        this->type = EResponseType::SPEAK;
    }if (type=="sentence") {
        this->type = EResponseType::SENTENCE;
    }if (type=="scene") {
        this->type = EResponseType::SCENE;
    }if (type=="response") {
        this->type = EResponseType::RESPONSE;
    }if (type=="print") {
        this->type = EResponseType::PRINT;
    }
}

void RRParser::CScriptRule::parseRule(std::ifstream &file)
{
    using namespace boost; //tokenizer again :)

    for (std::string line; safeGetline(file,line);) {
        line = line.substr(0,line.find("//"));
        char_separator<char> sep(" \t");

        tokenizer<char_separator<char>> tokens(line,sep);


        std::vector<std::string> tokenList(tokens.begin(),tokens.end()); //we should have list of tokens here

        if (tokenList.size()>0) {
            if(tokenList[0]=="{")
                continue; //start of the rule
            if(tokenList[0]=="}")
                break; //end of the rule

            //We're in rule definiton now

            if(tokenList[0]=="criteria") {
                for (auto token=tokenList.begin()+1;token!=tokenList.end();++token) {
                    std::string name = *(token);
                    stripQuotes(name);
                    referencedCriteria.push_back(name);
                }

            } else if(tokenList[0]=="response") {
                for (auto token=tokenList.begin()+1;token!=tokenList.end();++token) {
                    std::string name = *(token);
                    stripQuotes(name);
                    referencedResponses.push_back(name);
                }
            } else if(tokenList[0]=="matchonce") {
                triggerOnce=true;
            } else {
                CScriptCriterion criterion;
                criterion.parseCriterion(tokenList);
                anonymousCriteria.push_back(criterion);
            }

        }


    }

    return;
}

void RRParser::CScriptCriterion::parseCriterion(std::vector<std::string> &flags)
{
    // matchKey and Match value in that order.
    if (flags.size() < 1) {
        return;
    }
    std::string matchKey = flags[0];
    std::string matchValue = "";
    if (flags.size() > 1) {
         matchValue = flags[1];
    }
    stripQuotes(matchKey);stripQuotes(matchValue);
    this->matchKey=matchKey;this->matchValue=matchValue;
    if (flags.size() < 2) {
        return;
    }
    std::vector optionals(flags.begin()+2,flags.end());
    for(auto token = optionals.begin();token!=optionals.end();token++) {
        std::string flag = *token;
        stripQuotes(flag);
        if (flag.compare("weight")==0) {
            this->weight = std::stof(*(++token));
        }
        if (flag.compare("required")==0) {
            this->required=true;
        }
    }
}
