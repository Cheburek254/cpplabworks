#include <iostream>
#include <iterator>
#include <vector>
/*
bool IsTwiceSequence(????) {
    implementation
}
*/
template <typename Iterator>
bool IsTwiceSequence(Iterator begin, Iterator end){
   typename std::iterator_traits<Iterator>::difference_type  dist = std::distance(begin, end);
   if (dist <= 0 || dist%2 != 0){
   	   return false;
   }
   Iterator copy = begin;
   std::advance(copy, dist/2);
   while(copy != end){
	   if (*begin != * copy){
	        return false;
	    }
	   ++copy;
	   ++begin;
   }
   return true;
}


int main(int, char**){

}
