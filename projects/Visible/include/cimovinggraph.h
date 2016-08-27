
#ifndef CI_MOVING_GRAPH
#define CI_MOVING_GRAPH

#include "cinder/Shape2d.h"
#include "ciUIWidget.h"
#include <boost/algorithm/minmax.hpp>
#include <boost/algorithm/minmax_element.hpp>

class ciMovingGraph : public ciUIWidget
{
public:    
    ciMovingGraph(float x, float y, float w, float h, vector<float> _buffer, int _bufferSize, float _min, float _max, string _name)
    {
        rect = new ciUIRectangle(x,y,w,h); 
        init(w, h, _buffer, _bufferSize, _min, _max, _name);
    }
    
    ciMovingGraph (float w, float h, vector<float> _buffer, int _bufferSize, float _min, float _max, string _name)
    {
        rect = new ciUIRectangle(0,0,w,h); 
        init(w, h, _buffer, _bufferSize, _min, _max, _name);
    }    
    
    void init(float w, float h, vector<float> _buffer, int _bufferSize, float _min, float _max, string _name)
    {
		name = _name; 				
		kind = CI_UI_WIDGET_MOVINGGRAPH; 
		
		paddedRect = new ciUIRectangle(-padding, -padding, w+padding*2.0f, h+padding*2.0f);
		paddedRect->setParent(rect); 
		
        draw_fill = true; 
        
        buffer = _buffer;					//the widget's value
        
        bufferSize = (_bufferSize > _buffer.size() ) ? _buffer.size() : _bufferSize;
        m_begin = _buffer.begin();
        m_end = _buffer.begin();
        std::advance (m_end, bufferSize);
		max = _max; 
		min = _min; 		
		scale = rect->getHeight(); 
        inc = rect->getWidth()/((float)bufferSize-1.0f);         
    }
    
    virtual void drawFill()
    {
        if(draw_fill)
        {			
			if(draw_fill_highlight)
			{
                ci::gl::color(color_fill_highlight); 
			}        
			else 
			{
				ci::gl::color(color_fill); 		 	
			}
            
            ci::gl::pushMatrices();
            ci::gl::translate(rect->getX(),rect->getY()+scale);         
            shape.clear();
            
            vector<float>::const_iterator ticker = m_begin;
            shape.moveTo(0.0f, ci::lmap<float>(*ticker++, min, max, 0, scale));
            for (int i = 1; i < bufferSize; i++, ticker++)
            {			
                shape.lineTo(inc*(float)i, ci::lmap<float>(*ticker, min, max, 0, scale)); 
            }                
            ci::gl::draw(shape);
            ci::gl::popMatrices();                
        }
    }
    
	void setParent(ciUIWidget *_parent)
	{
		parent = _parent; 
	}
	
    void increment ()
    {
        if (m_end == buffer.end()) reset ();
        else  { m_begin++, m_end++; }
    }
    
    void seekToIndex (uint32 data_index)
    {
        set (data_index, bufferSize / 2);
    }
    
    
    vector<float> &getBuffer()
    {
        return buffer; 
    }
    
    void setBuffer(vector<float> _buffer)
    {
        buffer = _buffer; 
    }
    
protected:    //inherited: ciUIRectangle *rect; ciUIWidget *parent; 
    vector<float> buffer;
    float max, min, scale, inc; 
	int bufferSize;
    vector<float>::const_iterator m_begin, m_end;
    Shape2d shape; 
    void reset ()
    {
        m_begin = buffer.begin();
        m_end = buffer.begin();
        std::advance (m_end, bufferSize);
    }
    void set (uint32 index, uint32 offset)
    {
        m_begin = buffer.begin();
        std::advance (m_begin, bufferSize - offset);
        m_end = m_begin;
        std::advance (m_end, bufferSize);        
    }
    
}; 



#endif
