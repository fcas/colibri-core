#include "pattern.h"
#include "patternstore.h"
#include "SpookyV2.h" //spooky hash
#include <cstring>

/*****************************
* Colibri Core
*   by Maarten van Gompel
*   Centre for Language Studies
*   Radboud University Nijmegen
*
*
*   http://proycon.github.io/colibri-core
*   
*   Licensed under GPLv3
*****************************/

using namespace std;


unsigned char mainpatternbuffer[MAINPATTERNBUFFERSIZE+1];

const PatternCategory datacategory(const unsigned char * data, int maxbytes = 0) {
    PatternCategory category = NGRAM;
    int i = 0;
    unsigned int length;
    do {
        if ((maxbytes > 0) && (i >= maxbytes)) return category;
        const unsigned int cls = bytestoint(data + i, &length);
        i += length;
        if (cls == ClassDecoder::delimiterclass) {
            //end marker
            return category;
        } else if (cls == ClassDecoder::skipclass) {
            return SKIPGRAM;
        } else if (cls == ClassDecoder::flexclass) {
            return FLEXGRAM;;
        }
    } while (1);
}

const PatternCategory Pattern::category() const {
    return datacategory(data);
}

const PatternCategory PatternPointer::category() const {
    return datacategory(data, bytes);
}


const size_t Pattern::bytesize() const {
    //return the size of the pattern (in bytes)
    int i = 0;
    unsigned int length;
    do {
        const unsigned int cls = bytestoint(data + i, &length);
        i += length;
        if (cls == ClassDecoder::delimiterclass) {
            //end marker
            return i;
        }
    } while (1);
}

const size_t datasize(unsigned char * data, int maxbytes = 0) {
    //return the size of the pattern (in tokens)
    int i = 0;
    int n = 0;
    unsigned int length;
    do {
        if ((maxbytes > 0) && (maxbytes >= i)) {
            return n;
        }
        const unsigned int cls = bytestoint(data + i, &length);
        i += length;
        if (cls == ClassDecoder::delimiterclass) {
            return n;
        } else {
            n += 1;
        }
    } while (1);
}

const size_t Pattern::n() const {
    return datasize(data);
}

const size_t PatternPointer::n() const {
    return datasize(data, bytes);
}


bool Pattern::isgap(int index) const { //is the word at this position a gap? 
    int i = 0;
    int n = 0;
    do {
        const unsigned char c = data[i];
        if (c == ClassDecoder::delimiterclass) {
            //end marker
            return false;
        } else if ((c == ClassDecoder::skipclass)  || (c == ClassDecoder::flexclass)) {
            //FLEXMARKER is counted as 1, the minimum fill
            if (n == index) return true;
            i++;
            n++;
        } else if (c < 128) {
            //we have a size
            if (n == index) return false;
            i++;
            n++;
        } else {
            i++;
        }
    } while (1);
}

Pattern Pattern::toflexgram() const { //converts a fixed skipgram into a dynamic one, ngrams just come out unchanged
    //to be computed in bytes
    int i = 0;
    int j = 0;
    int copybytes = 0;
    bool skipgap = false;
    do {
        const unsigned char c = data[i++];
        if (j >= MAINPATTERNBUFFERSIZE) {
            std::cerr << "ERROR: toflexgram(): Patternbuffer size exceeded" << std::endl;
            throw InternalError();
        }
        if (copybytes) {
            mainpatternbuffer[j++] = c;
            copybytes--;
        } else if (copybytes == 0) {
            if (c == ClassDecoder::delimiterclass) {
                mainpatternbuffer[j++] = c;
                break;
            } else if (c == ClassDecoder::skipclass) {
                if (!skipgap) {
                    mainpatternbuffer[j++] = ClassDecoder::flexclass; //store a DYNAMIC GAP instead
                    skipgap = true; //skip next consecutive gap markers
                }
            } else {
                mainpatternbuffer[j++] = c;
                skipgap = false;
            }
        }
    } while (1);
    
    //copy from mainpatternbuffer
    return Pattern(mainpatternbuffer,j-1);
}

const size_t Pattern::hash() const {

    int s = 0;
    int i = 0;
    unsigned int length;
    do {
        const unsigned int cls = bytestoint(data + i, &length);
        i += length;
        if (cls == ClassDecoder::delimiterclass) {
            break;
        } else {
            s += cls;
        }
    } while (1);

    //clean, no markers (we don't want include markers except final \0
    //marker in our hash)
    if (sizeof(size_t) == 8) {
        return SpookyHash::Hash64((const void*) data , s);
    } else if (sizeof(size_t) == 4) {
        return SpookyHash::Hash32((const void*) data , s);
    }
}



void Pattern::write(ostream * out) const {
    const int s = bytesize();
    if (s > 0) {
        out->write( (char*) data , (int) s + 1); //+1 to include the \0 marker
    }
}

std::string datatostring(unsigned char * data, const ClassDecoder& classdecoder, int maxbytes = 0) {
    std::string result = ""; 
    int i = 0;
    int gapsize = 0;
    unsigned int length; 
    do {
        if ((maxbytes > 0) && (i >= maxbytes)) {
            return result;
        }
        const unsigned int cls = bytestoint(data + i, &length);
        if (length == 0) {
            cerr << "ERROR: Class length==0, shouldn't happen" << endl;
            throw InternalError();
        }
        i += length;
        if (!result.empty()) result += " ";
        if ((maxbytes == 0) && (cls == ClassDecoder::delimiterclass)) {
            return result;
        } else if (classdecoder.hasclass(cls)) {
            result += classdecoder[cls];
        } else {
            result += "{?}";
        }
    } while (1);
    return result;  
}


std::string Pattern::tostring(const ClassDecoder& classdecoder) const {
    return datatostring(data, classdecoder);
}


std::string PatternPointer::tostring(const ClassDecoder& classdecoder) const {
    return datatostring(data, classdecoder, bytesize());
}

bool dataout(unsigned char * data, int maxbytes = 0) {
    int i = 0;
    unsigned int length;
    do {
        if ((maxbytes > 0) && (i >= maxbytes)) {
            return true;
        }
        const unsigned int cls = bytestoint(data + i, &length);
        i += length;
        if (cls == ClassDecoder::delimiterclass) {
            cerr << endl;
            return true;
        } else {
            cerr << cls << " ";
        }
    } while (1);
    return false;
}

bool Pattern::out() const { 
    return dataout(data);
}

bool PatternPointer::out() const { 
    return dataout(data, bytesize());
}

const bool Pattern::unknown() const {
    int i = 0;
    unsigned int length;
    do {
        const unsigned int cls = bytestoint(data + i, &length);
        i += length;
        if (cls == ClassDecoder::delimiterclass) {
            //end marker
            return false;
        } else if (cls == ClassDecoder::unknownclass) {
            return true;
        }
    } while (1);
}

vector<unsigned int> Pattern::tovector() const { 
    vector<unsigned int> v;
    int i = 0;
    unsigned int length;
    do {
        const unsigned int cls = bytestoint(data + i, &length);
        i += length;
        if (cls == ClassDecoder::delimiterclass) {
            return v;
        } else {
            v.push_back(cls);
        }
    } while (1);
}

void readanddiscardpattern(std::istream * in) {
    unsigned char c;
    do {
        const unsigned int cls = bytestoint(in);
        if (cls == ClassDecoder::delimiterclass) {
            return;
        }
    } while (1);
}

void readanddiscardpattern_v1(std::istream * in) {
    unsigned char c;
    do {
        in->read( (char*) &c, sizeof(char));
        if (c == 0) {
            return;
        } else if (c < 128) {
            //we have a size
            const size_t pos = in->tellg();
            in->seekg(pos + c);
        } else {
            //we have a marker
        }
    } while (1);
}



Pattern::Pattern(std::istream * in, bool ignoreeol, const unsigned char version, bool debug) {
    if (version == 2) {
        //stage 1 -- get length
        unsigned char c = 0;

        std::streampos beginpos = 0;
        bool gotbeginpos = false;

        //stage 1 -- get length
        int length = 0;
        do {
            if (in->good()) {
                if (!gotbeginpos) {
                    beginpos = in->tellg();
                    gotbeginpos = true;
                }
                in->read( (char* ) &c, sizeof(char));
                if (debug) std::cerr << "DEBUG read1=" << (int) c << endl;
            } else {
                if (ignoreeol) {
                    break;
                } else {
                    std::cerr << "WARNING: Unexpected end of file (stage 1, length=" << length << "), no EOS marker found (adding and continuing)" << std::endl;
                    in->clear(); //clear error bits
                    break;
                }
            }
            length++;
            if (c == 0) {
                if (!ignoreeol) break;
            }
        } while (1);

        if (length == 0) {
            std::cerr << "ERROR: Attempting to read pattern from file, but file is empty?" << std::endl;
            throw InternalError();
        }

        //allocate buffer
        if (c == 0) {
            data  = new unsigned char[length];
        } else {
            data  = new unsigned char[length+1];
        }

        //stage 2 -- read buffer
        int i = 0;
        if (debug) std::cerr << "STARTING STAGE 2: BEGINPOS=" << beginpos << ", LENGTH=" << length << std::endl;
        if (!gotbeginpos) {
            std::cerr << "ERROR: Invalid position in input stream whilst Reading pattern" << std::endl;
            throw InternalError();
        }
        in->seekg(beginpos, ios::beg);
        std::streampos beginposcheck = in->tellg();
        if ((beginposcheck != beginpos) && (beginposcheck >= 0xffffffffffffffff)) {
            std::cerr << "ERROR: Resetting read pointer for stage 2 failed! (" << (unsigned long) beginposcheck << " != " << (unsigned long) beginpos << ")" << std::endl;
            throw InternalError();
        } else if (!in->good()) {
            std::cerr << "ERROR: After resetting readpointer for stage 2, istream is not 'good': eof=" << (int) in->eof() << ", fail=" << (int) in->fail() << ", badbit=" << (int) in->bad() << std::endl;
            throw InternalError();
        }
        while (i < length) {
            if (in->good()) {
                in->read( (char* ) &c, sizeof(char));
                if (debug) std::cerr << "DEBUG read2=" << (int) c << endl;
            } else {
                std::cerr << "ERROR: Invalid pattern data, unexpected end of file (stage 2,i=" << i << ",length=" << length << ",beginpos=" << beginpos << ",eof=" << (int) in->eof() << ",fail=" << (int) in->fail() << ",badbit=" << (int) in->bad() << ")" << std::endl;
                throw InternalError();
            }
            data[i++] = c;
            if ((c == 0) && (!ignoreeol)) break;
        }

        if (c != 0) { //add endmarker
            data[i++] = 0;
        }

        if (debug) std::cerr << "DEBUG: DONE READING PATTERN" << std::endl;

        //if this is the end of file, we want the eof bit set already, so we try to
        //read one more byte (and wind back if succesful):
        if (in->good()) {
            if (debug) std::cerr << "DEBUG: (TESTING EOF)" << std::endl;
            in->read( (char* ) &c, sizeof(char));
            if (in->good()) in->unget();
        }

    } else if (version == 1) { 
        data = convert_v1_v2(in, ignoreeol, debug);
    } else {
        std::cerr << "ERROR: Unknown version " << (int) version << std::endl;
        throw InternalError();
    }
    
}


/*
Pattern::Pattern(std::istream * in, unsigned char * buffer, int maxbuffersize, bool ignoreeol, const unsigned char version, bool debug) {
    //read pattern using a buffer
    if (version == 2) {
    } else if (version == 1){ 
        int i = 0;
        int readingdata = 0;
        unsigned char c = 0;
        do {
            if (in->good()) {
                in->read( (char* ) &c, sizeof(char));
                if (debug) std::cerr << "DEBUG read=" << (int) c << endl;
            } else {
                std::cerr << "ERROR: Invalid pattern data, unexpected end of file i=" << i << std::endl;
                throw InternalError();
            }
            if (i >= maxbuffersize) {
                std::cerr << "ERROR: Pattern read would exceed supplied buffer size (" << maxbuffersize << ")! Aborting prior to segmentation fault..." << std::endl;
                throw InternalError();
            }
            buffer[i++] = c;
            if (readingdata) {
                readingdata--;
            } else {
                if (c == ENDMARKER) {
                    if (!ignoreeol) break;
                } else if (c < 128) {
                    //we have a size
                    if (c == 0) {
                        std::cerr << "ERROR: Pattern length is zero according to input stream.. not possible! (stage 2)" << std::endl;
                        throw InternalError();
                    } else {
                        readingdata = c;
                    }
                }
            }
        } while (1);

        if (c != ENDMARKER) { //add endmarker
            buffer[i++] = ENDMARKER;
        }


        if (debug) std::cerr << "DEBUG: Copying from buffer" << std::endl;

        data  = new unsigned char[i];
        for (int j = 0; j < i; j++) {
            data[j] = buffer[j];
        }
        
        if (debug) std::cerr << "DEBUG: DONE READING PATTERN" << std::endl;
    } else {
        std::cerr << "ERROR: Unknown version " << (int) version << std::endl;
        throw InternalError();
    }

}
*/

Pattern::Pattern(const unsigned char * dataref, const int _size) {
    data = new unsigned char[_size+1];
    for (int i = 0; i < _size; i++) {
        data[i] = dataref[i];
    }
    data[_size] = ClassEncoder::delimiterclass;
}


void Pattern::set(const unsigned char * dataref, const int _size) {
    if (data != NULL) {
        delete[] data;
        data = NULL;
    }
    data = new unsigned char[_size+1];
    for (int i = 0; i < _size; i++) {
        data[i] = dataref[i];
    }
    data[_size] = ClassEncoder::delimiterclass;
}


Pattern::Pattern(const Pattern& ref, int begin, int length) { //slice constructor
    //to be computed in bytes
    int begin_b = 0;
    int length_b = 0;

    int i = 0;
    int n = 0;
    do {
        const unsigned char c = ref.data[i];
        
        if ((n - begin == length) || (c == ClassDecoder::delimiterclass)) {
            length_b = i - begin_b;
            break;
        } else if (c < 128) {
            //we have a token
            n++; 
            i++;
            if (n == begin) begin_b = i;
        }
    } while (1);

    const unsigned char _size = length_b + 1;
    data = new unsigned char[_size];
    int j = 0;
    for (int i = begin_b; i < begin_b + length_b; i++) {
        data[j++] = ref.data[i];
    }
    data[j++] = ClassDecoder::delimiterclass;
}


PatternPointer::PatternPointer(const Pattern& ref, int begin, int length) { //slice constructor
    //to be computed in bytes
    int begin_b = 0;
    int length_b = 0;

    int i = 0;
    int n = 0;
    do {
        const unsigned char c = ref.data[i];
        
        if ((n - begin == length) || (c == ClassDecoder::delimiterclass)) {
            length_b = i - begin_b;
            break;
        } else if (c < 128) {
            //we have a token
            n++; 
            i++;
            if (n == begin) begin_b = i;
        }
    } while (1);

    data = ref.data + begin_b;
    if (length_b > 255) {
        std::cerr << "ERROR: Pattern too long for pattern pointer [length_b=" << length_b << ",begin=" << begin << ",length=" << length << ", reference_length_b=" << ref.bytesize() << "]  (did you set MAXLENGTH (-l)?)" << std::endl;
        std::cerr << "Reference=";
        ref.out();
        std::cerr << std::endl;
        throw InternalError();
    }
    bytes = length_b;
    
}



PatternPointer::PatternPointer(const PatternPointer& ref, int begin, int length) { //slice constructor
    //to be computed in bytes
    int begin_b = 0;
    int length_b = 0;

    int i = 0;
    int n = 0;
    do {
        if (i >= ref.bytes) {
            length_b = i - begin_b;
            break;
        }
        const unsigned char c = ref.data[i];
        
        if ((n - begin == length) || (c == ClassDecoder::delimiterclass)) {
            length_b = i - begin_b;
            break;
        } else if (c < 128) {
            //we have a token
            n++; 
            i++;
            if (n == begin) begin_b = i;
        }
    } while (1);

    data = ref.data + begin_b;
    bytes = length_b;
}

Pattern::Pattern(const Pattern& ref) { //copy constructor
    const int s = ref.bytesize();
    data = new unsigned char[s + 1];
    for (int i = 0; i < s; i++) {
        data[i] = ref.data[i];
    }
    data[s] = ClassDecoder::delimiterclass;
}

Pattern::Pattern(const PatternPointer& ref) { //constructor from patternpointer
    if (ref.bytesize() > 255) {
        std::cerr << "ERROR: Pattern too long for pattern pointer (Pattern from PatternPointer)" << std::endl;
        throw InternalError();
    }
    data = new unsigned char[ref.bytesize() + 1];
    for (unsigned int i = 0; i < ref.bytesize(); i++) {
        data[i] = ref.data[i];
    }
    data[ref.bytesize()] = ClassDecoder::delimiterclass;
}

Pattern::~Pattern() {
    if (data != NULL) {
        delete[] data;
        data = NULL;
    }
}


bool Pattern::operator==(const Pattern &other) const {
        const int s = bytesize();
        if (bytesize() == other.bytesize()) {
            for (int i = 0; i < s+1; i++) { //+1 for endmarker
                if (data[i] != other.data[i]) return false;
            }
            return true;
        } else {
            return false;
        }        
}

bool Pattern::operator!=(const Pattern &other) const {
    return !(*this == other);
}

bool Pattern::operator<(const Pattern & other) const {
    const int s = bytesize();
    const int s2 = other.bytesize();
    for (int i = 0; (i <= s && i <= s2); i++) {
        if (data[i] < other.data[i]) {
            return true;
        } else if (data[i] > other.data[i]) {
            return false;
        }
    }
    return (s < s2);
}

bool Pattern::operator>(const Pattern & other) const {
    return (other < *this);
}

void Pattern::operator =(const Pattern & other) { 
    //delete old data
    if (data != NULL) {
        delete[] data;
        data = NULL;
    } else if (data == other.data) {
        //nothing to do
        return;
    }
    
    //set new data
    const int s = other.bytesize();        
    data = new unsigned char[s+1];   
    for (int i = 0; i < s; i++) {
        data[i] = other.data[i];
    }  
    data[s] = ClassDecoder::delimiterclass;
}

Pattern Pattern::operator +(const Pattern & other) const {
    const int s = bytesize();
    const int s2 = other.bytesize();
    unsigned char buffer[s+s2]; 
    for (int i = 0; i < s; i++) {
        buffer[i] = data[i];
    }
    for (int i = 0; i < s2; i++) {
        buffer[s+i] = other.data[i];
    }
    return Pattern(buffer, s+s2);
}


int Pattern::find(const Pattern & pattern) const { //returns the index, -1 if not fount 
    const int s = bytesize();
    const int s2 = pattern.bytesize();
    if (s2 > s) return -1;
    
    for (int i = 0; i < s; i++) {
        if (data[i] == pattern.data[0]) {
            bool match = true;
            for (int j = 0; j < s2; j++) {
                if (data[i+j] != pattern.data[j]) {
                    match = false;
                    break;
                }
            }
            if (match) return i;
        }
    }
    return -1;
}

bool Pattern::contains(const Pattern & pattern) const {
    return (find(pattern) != -1);
}

int Pattern::ngrams(vector<Pattern> & container, const int n) const { //return multiple ngrams, also includes skipgrams!
    const int _n = this->n();
    if (n > _n) return 0;
    int found = 0;
    for (int i = 0; i < (_n - n) + 1; i++) {
        container.push_back( Pattern(*this,i,n));
        found++;
    }
    return found;
}   

int Pattern::ngrams(vector<PatternPointer> & container, const int n) const { //return multiple ngrams, also includes skipgrams!
    const int _n = this->n();
    if (n > _n) return 0;
    int found = 0;
    for (int i = 0; i < (_n - n) + 1; i++) {
        container.push_back(  PatternPointer(*this,i,n));
        found++;
    }
    return found;
}   

int PatternPointer::ngrams(vector<PatternPointer> & container, const int n) const { //return multiple ngrams, also includes skipgrams!
    const int _n = this->n();
    if (n > _n) return 0;
    int found = 0;
    for (int i = 0; i < (_n - n) + 1; i++) {
        container.push_back(  PatternPointer(*this,i,n));
        found++;
    }
    return found;
}   

int Pattern::ngrams(vector<pair<Pattern,int>> & container, const int n) const { //return multiple ngrams, also includes skipgrams!
    const int _n = this->n();
    if (n > _n) return 0;
    
    int found = 0;
    for (int i = 0; i < (_n - n)+1; i++) {
        container.push_back( pair<Pattern,int>(Pattern(*this,i,n),i) );
        found++;
    }
    return found;
}   

int Pattern::ngrams(vector<pair<PatternPointer,int>> & container, const int n) const { //return multiple ngrams, also includes skipgrams!
    const int _n = this->n();
    if (n > _n) return 0;
    
    int found = 0;
    for (int i = 0; i < (_n - n)+1; i++) {
        container.push_back( pair<PatternPointer,int>(PatternPointer(*this,i,n),i) );
        found++;
    }
    return found;
}   


int PatternPointer::ngrams(vector<pair<PatternPointer,int>> & container, const int n) const { //return multiple ngrams, also includes skipgrams!
    const int _n = this->n();
    if (n > _n) return 0;
    
    int found = 0;
    for (int i = 0; i < (_n - n)+1; i++) {
        container.push_back( pair<PatternPointer,int>(PatternPointer(*this,i,n),i) );
        found++;
    }
    return found;
}   

int Pattern::subngrams(vector<Pattern> & container, int minn, int maxn) const { //also includes skipgrams!
    const int _n = n();
    if (maxn > _n) maxn = _n;
    if (minn > _n) return 0;
    int found = 0;
    for (int i = minn; i <= maxn; i++) {
        found += ngrams(container, i);
    }
    return found;
}

int Pattern::subngrams(vector<PatternPointer> & container, int minn, int maxn) const { //also includes skipgrams!
    const int _n = n();
    if (maxn > _n) maxn = _n;
    if (minn > _n) return 0;
    int found = 0;
    for (int i = minn; i <= maxn; i++) {
        found += ngrams(container, i);
    }
    return found;
}

int PatternPointer::subngrams(vector<PatternPointer> & container, int minn, int maxn) const { //also includes skipgrams!
    const int _n = n();
    if (maxn > _n) maxn = _n;
    if (minn > _n) return 0;
    int found = 0;
    for (int i = minn; i <= maxn; i++) {
        found += ngrams(container, i);
    }
    return found;
}

int Pattern::subngrams(vector<pair<Pattern,int>> & container, int minn, int maxn) const { //also includes skipgrams!
    const int _n = n();
    if (maxn > _n) maxn = _n;
    if (minn > _n) return 0;
    int found = 0;
    for (int i = minn; i <= maxn; i++) {
        found += ngrams(container, i);
    }
    return found;
}

int Pattern::subngrams(vector<pair<PatternPointer,int>> & container, int minn, int maxn) const { //also includes skipgrams!
    const int _n = n();
    if (maxn > _n) maxn = _n;
    if (minn > _n) return 0;
    int found = 0;
    for (int i = minn; i <= maxn; i++) {
        found += ngrams(container, i);
    }
    return found;
}

int PatternPointer::subngrams(vector<pair<PatternPointer,int>> & container, int minn, int maxn) const { //also includes skipgrams!
    const int _n = n();
    if (maxn > _n) maxn = _n;
    if (minn > _n) return 0;
    int found = 0;
    for (int i = minn; i <= maxn; i++) {
        found += ngrams(container, i);
    }
    return found;
}

int Pattern::parts(vector<pair<int,int>> & container) const { 
    //to be computed in bytes
    int partbegin = 0;
    int partlength = 0;

    int found = 0;
    int i = 0;
    int n = 0;
    do {
        const unsigned char c = data[i];
        
        if (c == ClassDecoder::delimiterclass) {
            partlength = n - partbegin;
            if (partlength > 0) {
                container.push_back(pair<int,int>(partbegin,partlength));
                found++;
            }
            break;
        } else if ((c == ClassDecoder::skipclass) || (c == ClassDecoder::flexclass)) {        
            partlength = n - partbegin;
            if (partlength > 0) {
                container.push_back(pair<int,int>(partbegin,partlength));
                found++;
            }
            i++;
            n++; 
            partbegin = n; //for next part
        } else if (c < 128) {
            //we have a size
            i++;
            n++;
        } else {
            //we have another marker
            i++;
        }
    } while (1);
    return found;
}

int Pattern::parts(vector<Pattern> & container) const {
    vector<pair<int,int>> partoffsets; 
    int found = parts(partoffsets);

    for (vector<pair<int,int>>::iterator iter = partoffsets.begin(); iter != partoffsets.end(); iter++) {
        const int begin = iter->first;
        const int length = iter->second;
        Pattern pattern = Pattern(*this,begin, length);
        container.push_back( pattern );
    }
    return found;
}

const unsigned int Pattern::skipcount() const { 
    int count = 0;
    int i = 0;
    do {
        const unsigned char c = data[i];
        if (c == ClassDecoder::delimiterclass) {
            //end marker
            return count;
        } else if (((c == ClassDecoder::skipclass) || (c == ClassDecoder::flexclass)) && ((i > 0) || ((data[i-1] != ClassDecoder::skipclass) && (data[i-1] != ClassDecoder::flexclass))))   {
            //we have a marker
            count++;
            i++;
        } else {
            i++;
        }
    } while (1); 
}

int Pattern::gaps(vector<pair<int,int> > & container) const { 
    vector<pair<int,int> > partscontainer;
    parts(partscontainer);

    const int _n = n();
    const int bs = bytesize();
    

    //compute inverse:
    int begin = 0;
    for (vector<pair<int,int>>::iterator iter = partscontainer.begin(); iter != partscontainer.end(); iter++) {
        if (iter->first > begin) {
            container.push_back(pair<int,int>(begin,iter->first - begin));
        }
        begin = iter->first + iter->second;
    }
    if (begin != _n) {
        container.push_back(pair<int,int>(begin,_n - begin));
    }
    
    int endskip = 0;
    for (int i = bs; i > 0; i--) {
        if ((data[i] == ClassDecoder::skipclass) || (data[i] == ClassDecoder::flexclass)) {
            endskip++;
        } else {
            break;
        }
    } 

    if (endskip) container.push_back(pair<int,int>(_n - endskip,endskip));
    return container.size();
}

Pattern Pattern::extractskipcontent(const Pattern & instance) const {
    if (this->category() == FLEXGRAM) {
        cerr << "Extractskipcontent not supported on Pattern with dynamic gaps!" << endl;
        throw InternalError();
    }
    if (this->category() == NGRAM) {
        cerr << "Extractskipcontent not supported on Pattern without gaps!" << endl;
        throw InternalError();
    }
    if (instance.n() != n()) {
        cerr << "WARNING: Extractskipcontent(): instance.n() != skipgram.n(), " << (int) instance.n() << " != " << (int) n() << endl;
        cerr << "INSTANCE: " << instance.out() << endl;
        cerr << "SKIPGRAM: " << out() << endl;
        throw InternalError();
    }
    std::vector<std::pair<int,int> > gapcontainer;
    gaps(gapcontainer);
    
    unsigned char a = ClassDecoder::skipclass;
    const Pattern skip = Pattern(&a,1);

    std::vector<std::pair<int,int> >::iterator iter = gapcontainer.begin();
    Pattern pattern = Pattern(instance,iter->first, iter->second);
    int cursor = iter->first + iter->second;
    iter++;
    while (iter != gapcontainer.end()) {  
        if (cursor > 0) {
            const int skipsize = iter->first - cursor;
            for (int i = 0; i < skipsize; i++) {
                pattern = pattern + skip;
            }
        }    
        Pattern subngram = Pattern(instance,iter->first, iter->second);
        pattern = pattern + subngram;
        cursor = iter->first + iter->second;
        iter++;
    }
    return pattern;
}

bool Pattern::instanceof(const Pattern & skipgram) const { 
    //Is this an instantiation of the skipgram?
    //Instantiation is not necessarily full, aka: A ? B C is also an instantiation
    //of A ? ? C
    if (this->category() == FLEXGRAM) return false;
    if (skipgram.category() == NGRAM) return (*this) == skipgram;

    if (skipgram.category() == FLEXGRAM) {
        //DYNAMIC SKIPGRAM
        //TODO: NOT IMPLEMENTED YET!!
       return false;
    } else {
        //FIXED SKIPGRAM
        const unsigned int _n = n();
        if (skipgram.n() != _n) return false;

        for (unsigned int i = 0; i < _n; i++) {
            const Pattern token1 = Pattern(skipgram, i, 1);
            const Pattern token2 = Pattern(*this, i, 1);
            if ((token1 != token2) && (token1.category() != SKIPGRAM)) return false;
        }
        return true;
    }

}


Pattern Pattern::replace(int begin, int length, const Pattern & replacement) const {
    const int _n = n();
    if (begin + length > _n) {
        cerr << "ERROR: Replacing slice " << begin << "," << length << " in a pattern of length " << _n << "! Out of bounds!" << endl;
        throw InternalError();
    }

    if (begin > 0) {
        Pattern p = Pattern(*this,0,begin) + replacement;
        if (begin+length != _n) {
            return p + Pattern(*this,begin+length,_n - (begin+length));
        } else {
            return p;
        }
    } else {
        Pattern p = replacement;
        if (begin+length != _n) {
            return p + Pattern(*this,begin+length,_n - (begin+length));
        } else {
            return p;
        }
    }
}


Pattern Pattern::addskip(const std::pair<int,int> & gap) const {
    //Returns a pattern with the specified span replaced by a fixed skip
    const unsigned int _n = n();
    const Pattern replacement = Pattern(gap.second);
    Pattern pattern = replace(gap.first, gap.second, replacement);
    if (pattern.n() != _n) {
        std::cerr << "ERROR: addskip(): Pattern length changed from " << _n << " to " << pattern.n() << " after substituting slice (" << gap.first << "," <<gap.second << ")" << std::endl;
        throw InternalError();
    }
    return pattern;
}

Pattern Pattern::addskips(const std::vector<std::pair<int,int> > & gaps) const {
    //Returns a pattern with the specified spans replaced by fixed skips
    const unsigned int _n = n();
    Pattern pattern = *this; //needless copy?
    for (vector<pair<int,int> >::const_iterator iter = gaps.begin(); iter != gaps.end(); iter++) {
        const Pattern replacement = Pattern(iter->second);
        pattern = pattern.replace(iter->first, iter->second, replacement);
        if (pattern.n() != _n) {
            std::cerr << "ERROR: addskip(): Pattern length changed from " << _n << " to " << pattern.n() << " after substituting slice (" << iter->first << "," <<iter->second << ")" << std::endl;
            throw InternalError();
        }
    }
    return pattern;
}

Pattern Pattern::addflexgaps(const std::vector<std::pair<int,int> > & gaps) const {
    //Returns a pattern with the specified spans replaced by fixed skips
    Pattern pattern = *this; //needless copy? 
    for (vector<pair<int,int> >::const_iterator iter = gaps.begin(); iter != gaps.end(); iter++) {
        pattern = pattern.replace(iter->first, iter->second, FLEXPATTERN);
    }
    return pattern;
}


Pattern Pattern::reverse() const { //TODO: convert to v2
    const unsigned char _size = bytesize() + 1;
    unsigned char * newdata = new unsigned char[_size];

    //set endmarker
    newdata[_size - 1] = ClassDecoder::delimiterclass;

    //we fill the newdata from right to left
    unsigned char cursor = _size - 1;

    int i = 0;
    do {
        const unsigned char c = data[i];
        if (c == ClassDecoder::delimiterclass) {
            //end marker
            break;
        } else if (c < 128) {
            //we have a size
            cursor = cursor - c - 1; //move newdata cursor to the left (place of insertion)
            strncpy((char*) newdata + cursor, (char*) data + i, c + 1);
            i += c + 1;
        } else {
            //we have another marker
            newdata[--cursor] = c;
            i++;
        }
    } while (1);

    Pattern rev = Pattern(newdata, _size);
    delete[] newdata;
    return rev;
}


IndexedCorpus::IndexedCorpus(std::istream *in, bool debug){
    this->load(in, debug);
}

IndexedCorpus::IndexedCorpus(std::string filename, bool debug){
    this->load(filename, debug);
}


void IndexedCorpus::load(std::istream *in, bool debug) {
    int sentence = 0;
    while (in->good()) {
        sentence++;
        Pattern line = Pattern(in);
        int linesize = line.size();
        for (int i = 0; i < linesize; i++) {
            const Pattern unigram = line[i];
            const IndexReference ref = IndexReference(sentence,i);
            data.push_back(IndexPattern(ref,unigram));
        }
    }
    if (debug) cerr << "Loaded " << sentence << " sentences" << endl;
    data.shrink_to_fit();
}


void IndexedCorpus::load(std::string filename, bool debug) {
    std::ifstream * in = new std::ifstream(filename.c_str());
    if (!in->good()) {
        std::cerr << "ERROR: Unable to load file " << filename << std::endl;
        throw InternalError();
    }
    this->load( (std::istream *) in, debug);
    in->close();
    delete in;
}

Pattern IndexedCorpus::getpattern(const IndexReference & begin, int length) const {
    //warning: will segfault if mainpatternbuffer overflows!!
    //length in tokens
    //
    //std::cerr << "getting pattern " << begin.sentence << ":" << begin.token << " length " << length << std::endl;
    const_iterator iter = this->find(begin);
    unsigned char * buffer = mainpatternbuffer;
    unsigned int classlength;
    int i = 0;
    while (i < length) {
        if ((iter == this->end()) || (iter->ref != begin + i)) {
            //std::cerr << "ERROR: Specified index " << (begin + i).tostring() << " (pivot " << begin.tostring() << ", offset " << i << ") does not exist"<< std::endl;
            throw KeyError();
        }
        classlength = inttobytes(buffer, iter->cls);
        buffer += classlength;
        i++;
        iter++;
    }
    if (buffer == mainpatternbuffer) {
        //std::cerr << "ERROR: Specified index " << begin.tostring() << " does not exist"<< std::endl;
        throw KeyError();
    }
    unsigned int buffersize = buffer - mainpatternbuffer; //pointer arithmetic
    return Pattern(mainpatternbuffer, buffersize);
}

std::vector<IndexReference> IndexedCorpus::findpattern(const Pattern & pattern, int maxmatches) {
    //far more inefficient than a pattrn model obviously
    std::vector<IndexReference> result;
    const int _n = pattern.size();
    if (_n == 0) return result;

    IndexReference ref;
    int i = 0;
    bool moved = false;
    Pattern matchunigram = pattern[i];
    for (iterator iter = this->begin(); iter != this->end(); iter++) {
        Pattern unigram = iter->pattern(); 
        iter->pattern().out();
        if (matchunigram == unigram) {
            if (i ==0) ref = iter->ref;
            i++;
            if (i == _n) {
                result.push_back(ref);
                if ((maxmatches != 0) && (result.size() == (unsigned int) maxmatches)) break;
            }
            matchunigram = pattern[i];
            moved = true;
        } else {
            i = 0;
            if (moved) matchunigram = pattern[i];
            moved = false;
        }
    }
    return result;
}

int IndexedCorpus::sentencelength(int sentence) const {
    IndexReference ref = IndexReference(sentence, 0);
    int length = 0;
    for (const_iterator iter = this->find(ref); iter != this->end(); iter++) {
        if (iter->ref.sentence != (unsigned int) sentence) return length;
        length++;
    }
    return length;
}

unsigned int IndexedCorpus::sentences() const {
    unsigned int max = 0;
    for (const_iterator iter = this->begin(); iter != this->end(); iter++) {
        if (iter->ref.sentence > max) max = iter->ref.sentence;
    }
    return max;
}

Pattern IndexedCorpus::getsentence(int sentence) const { 
    return getpattern(IndexReference(sentence,0), sentencelength(sentence));
}


Pattern patternfromfile(const std::string & filename) {//helper function to read pattern from file, mostly for Cython
    std::ifstream * in = new std::ifstream(filename.c_str());
    if (!in->good()) {
        std::cerr << "ERROR: Unable to load file " << filename << std::endl;
        throw InternalError();
    }
    Pattern p = Pattern( (std::istream *) in, true);
    in->close();
    delete in;
    return p;
}
