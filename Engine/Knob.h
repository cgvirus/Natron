//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef KNOB_H
#define KNOB_H

#include <vector>
#include <string>
#include <map>
#include <cfloat>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <QtCore/QMutex>
#include <QtCore/QStringList>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "Global/GlobalDefines.h"
#include "Global/AppManager.h"

#include "Engine/Variant.h"

class Knob;
class holder;
class DockablePanel;
class AppInstance;

namespace Natron {
    class LibraryBinary;
}

/******************************KNOB_FACTORY**************************************/
//Maybe the factory should move to a separate file since it is used to create KnobGui aswell
class KnobGui;
class KnobHolder;
class KnobFactory{
    
    std::map<std::string,Natron::LibraryBinary*> _loadedKnobs;
    
public:
    KnobFactory();
    
    ~KnobFactory();
    
    const std::map<std::string,Natron::LibraryBinary*>& getLoadedKnobs() const {return _loadedKnobs;}
    
    Knob* createKnob(const std::string& id, KnobHolder* holder, const std::string& description,int dimension = 1) const;
    
    KnobGui* createGuiForKnob(Knob* knob,DockablePanel* container) const;
    
private:
    
    void loadKnobPlugins();
    
    void loadBultinKnobs();
    
};



/**
 * @brief A KeyFrame is a pair <time,value>. These are the value that are used
 * to interpolate an AnimationCurve. The _leftTangent and _rightTangent can be
 * used by the interpolation method of the curve.
**/
class KeyFrame : public QObject {
    
    Q_OBJECT
    
    Variant _value; /// the value held by the key
    double _time; /// a value ranging between 0 and 1
    std::pair<double,Variant> _leftTangent,_rightTangent;

public:

    KeyFrame(double time,const Variant& initialValue);

    ~KeyFrame(){}

signals:

    void keyFrameChanged();
};


class AnimationCurve : public QObject {

    Q_OBJECT
    
public:

    enum Interpolation{
        CONSTANT = 0,
        LINEAR = 1,
        CUBIC = 2,
        CATMULL_ROM = 3
    };

    AnimationCurve(Interpolation type);

    ~AnimationCurve(){}

    void setStartAndEnd(boost::shared_ptr<KeyFrame> start,boost::shared_ptr<KeyFrame> end){
        addControlPoint(start);
        addControlPoint(end);
    }

    void addControlPoint(boost::shared_ptr<KeyFrame> cp)
    {
        _controlPoints.push_back(cp);
        QObject::connect(cp.get(),SIGNAL(keyFrameChanged()),this,SIGNAL(curveChanged()));
    }

    Variant getValueAt(double t) const;

signals:

    void curveChanged();

private:

    Interpolation _interpolation;
    std::list< boost::shared_ptr<KeyFrame> > _controlPoints;
};



/******************************KNOB_BASE**************************************/
class Knob : public QObject
{
    Q_OBJECT
    
public:
    
    enum ValueChangedReason{USER_EDITED = 0,PLUGIN_EDITED = 1,STARTUP_RESTORATION = 2};

    typedef std::map<double,Variant>  Keys;

    
    Knob(KnobHolder*  holder,const std::string& description,int dimension = 1);
    
    virtual ~Knob();
    
    const std::string& getDescription() const { return _description; }
    
    const std::vector<U64>& getHashVector() const { return _hashVector; }
    
    KnobHolder*  getHolder() const { return _holder; }
    
    int getDimension() const {return _dimension;}
    
    /*Must return the type name of the knob. This name will be used by the KnobFactory
     to create an instance of this knob.*/
    virtual const std::string typeName()=0;
    
    virtual std::string serialize() const =0;
    
    void restoreFromString(const std::string& str);
    
    template<typename T>
    void setValue(T variant[],int count){
        for(int i = 0; i < count; ++i){
            setValueInternal(Variant(variant[i]),i);
        }
    }
    
    template<typename T>
    void setValue(const T &value,int dimension = 0) {
        setValueInternal(Variant(value),dimension);
    }
    
    /*Used to extract the value held by the knob.
     Derived classes should provide a more appropriate
     way to retrieve results in the expected type.*/
    template<typename T>
    T getValue(int dimension = 0) const {
        std::map<int,Variant>::const_iterator it = _value.find(dimension);
        assert(it != _value.end());
        return it->second.value<T>();
    }
    
    const std::map<int,Variant>& getMultiDimensionalValue() const {return _value;}

    /**
     * @brief Must return true if this knob can animate (i.e: if we can set different values depending on the time)
     * Some parameters cannot animate, for example a file selector. 
     **/
    virtual bool canAnimate() const = 0;
    
    /**
     * @brief Set the value of a knob for a specific dimension at a specific time. By default dimension
     * is 0. If the knob has only 1 dimension, it will set the dimension 0 regardless of the parameter dimension.
     * Otherwise, it will attempt to set a key for only the dimension 'dimensionIndex' of the knob.
     **/
    template<typename T>
    void setValueAtTime(double time,const T& value,int dimensionIndex = 0){
        assert(dimensionIndex < _dimension);
        setValueAtTimeInternal(time,Variant(value),dimensionIndex);
    }

    /**
     * @brief Set the value for the knob in all dimensions at time 'time'.
     **/
    template<typename T>
    void setValueAtTime(double time,T variant[],int count){
        for(int i = 0; i < count; ++i){
            setValueAtTimeInternal(time,Variant(variant[i]),i);
        }
    }

    /**
     * @brief Returns the value of the knob in a specific dimension at a specific time. If
     * the knob has no keys in this dimension it will return the value at the requested dimension
     * stored in _value. Type type parameter must be the type of the knob, i.e: double for a Double_Knob.
     * The type of the Variant can never be cast to QList<QVariant>.
     **/
    template<typename T>
    T getValueAtTime(double time,int dimension = 0){
        return getValueAtTimeInternal(time,dimension).value<T>();
    }

    /**
     * @brief Returns an ordered map of all the keys at a specific dimension.
     **/
    Knob::Keys getKeys(int dimension = 0);
    
    /*other must have exactly the same name*/
    void cloneValue(const Knob& other);
    
    void turnOffNewLine();
    
    void setSpacingBetweenItems(int spacing);
    
    void setEnabled(bool b);
    
    void setVisible(bool b);
    
    /*Call this to change the knob name. The name is not the text label displayed on 
     the GUI but what is passed to the valueChangedByUser signal. By default the
     name is the same as the description(i.e: the text label).*/
    void setName(const std::string& name){_name = QString(name.c_str());}
    
    std::string getName() const {return _name.toStdString();}
    
    void setParentKnob(Knob* knob){_parentKnob = knob;}
    
    Knob* getParentKnob() const {return _parentKnob;}
    
    int determineHierarchySize() const;
    
    bool isVisible() const {return _visible;}
    
    bool isEnabled() const {return _enabled;}
    
    void setIsInsignificant(bool b){_isInsignificant = b;}
    
    void turnOffUndoRedo() {_canUndo = false;}
    
    bool canBeUndone() const {return _canUndo;}
    
    bool isInsignificant() const {return _isInsignificant;}
    
    void setHintToolTip(const std::string& hint){_tooltipHint = hint;}
    
    const std::string& getHintToolTip() const {return _tooltipHint;}

    
public slots:
    
    /*Set the value of the knob but does NOT emit the valueChanged signal.
     This is called by the GUI hence does not change the value of any
     render thread storage.*/
    void onValueChanged(int dimension,const Variant& variant);
    
    void onKnobUndoneChange();
    
    void onKnobRedoneChange();
    
signals:
    
    void deleted();
    
    /*Emitted when the value is changed internally by a call to setValue*/
    void valueChanged(int dimension,const Variant&);

    void visible(bool);
    
    void enabled(bool);
    
    
protected:
    virtual void fillHashVector()=0; // function to add the specific values of the knob to the values vector.
    
    virtual void _restoreFromString(const std::string& str) =0;
    
    /*This function can be implemented if you want to clone more data than just the value
     of the knob. Cloning happens when a render request is made: all knobs values of the GUI
     are cloned into small copies in order to be sure they will not be modified further on.
     This function is useful if you need to copy for example an extra bit of information.
     e.g: the File_Knob has to copy not only the QStringList containing the file names, but 
     also the sequence it has parsed.*/
    virtual void cloneExtraData(const Knob& other){(void)other;}
    
    /*This function is called right after that the _value has changed
     but before any signal notifying that it has changed. It can be useful
     to do some processing to create new informations.
     e.g: The File_Knob parses the files list to create a mapping of 
     <time,file> .*/
    virtual void processNewValue(){}
    
    KnobHolder*  _holder;
    /* A variant storing all the values of the knob. <dimension,value>*/
    std::map<int,Variant> _value;
    std::vector<U64> _hashVector;
    int _dimension;
    
    
private:
    
    void updateHash();
    
    void setValueInternal(const Variant& v,int dimension);

    void setValueAtTimeInternal(double time,const Variant& v,int dimension);

    Variant getValueAtTimeInternal(double time, int dimension) const;
    
    std::string _description;//< the text label that will be displayed  on the GUI
    QString _name;//< the knob can have a name different than the label displayed on GUI.
                          //By default this is the same as _description but can be set by calling setName().
    bool _newLine;
    int _itemSpacing;
    
    Knob* _parentKnob;
    bool _visible;
    bool _enabled;
    bool _canUndo;
    bool _isInsignificant; //< if true, a value change will never trigger an evaluation
    std::string _tooltipHint;


    typedef std::map<int,Keys> MultiDimensionalKeys;

    /*A map storing for each dimension the keys of the knob. For instance a Double_Knob of dimension 2
    would have 2 Keys in its map, 1 for dimension 0 and 1 for dimension 1.
    The Keys are an ordered map of <time,value> where value is the type requested for the knob (i.e: double
    for a double knob).*/
    MultiDimensionalKeys _keys; /// the keys for a specific dimension
};

/**
 * @brief A Knob holder is a class that stores Knobs and interact with them in some way. 
 * It serves 2 purpose: 
 * 1) It automatically deletes the knobs, you don't have to manually call delete
 * 2) It calls a set of begin/end valueChanged whenever a knob value changed. It also
 * calls evaluate() which should then trigger an evaluation of the freshly changed value
 * (i.e force a new render).
 **/
class KnobHolder {
    
    AppInstance* _app;
    std::vector<Knob*> _knobs;
    bool _betweenBeginEndParamChanged;

public:
    friend class Knob;
    
    KnobHolder(AppInstance* appInstance):
        _app(appInstance)
      , _knobs()
      , _betweenBeginEndParamChanged(false){}
    
    virtual ~KnobHolder(){
        while(!_knobs.empty()) {
            delete _knobs.back();
        }
    }
    
    /**
     * @brief Clone each knob of "other" into this KnobHolder.
     * WARNING: other must have exactly the same number of knobs.
     **/
    void cloneKnobs(const KnobHolder& other);
    
    AppInstance* getApp() const {return _app;}
    
    /**
     * @brief Must be implemented to initialize any knob using the
     * KnobFactory.
     **/
    virtual void initializeKnobs() = 0;
    
    /**
     * @brief Must be implemented to evaluate a value change
     * made to a knob(e.g: force a new render). 
     * @param knob[in] The knob whose value changed.
     **/
    virtual void evaluate(Knob* knob,bool isSignificant) = 0;
    
    /**
     * @brief Should be implemented by any deriving class that maintains
     * a hash value based on the knobs.
     **/
    void invalidateHash();
    
    int getAppAge() const;
    
    const std::vector<Knob*>& getKnobs() const { return _knobs; }
    
    void beginValuesChanged(Knob::ValueChangedReason reason);
    
    void endValuesChanged(Knob::ValueChangedReason reason);
    
    void onValueChanged(Knob* k,Knob::ValueChangedReason reason);
    
protected:
    
    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void beginKnobsValuesChanged(Knob::ValueChangedReason reason){(void)reason;}
    
    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void endKnobsValuesChanged(Knob::ValueChangedReason reason){(void)reason;}
    
    /**
     * @brief Called whenever a param changes. It calls the virtual
     * portion paramChangedByUser(...) and brackets the call by a begin/end if it was
     * not done already.
     **/
    virtual void onKnobValueChanged(Knob* k,Knob::ValueChangedReason reason){(void)k;(void)reason;}

    
private:
    
    void triggerAutoSave();
    
    /*Add a knob to the vector. This is called by the
     Knob class.*/
    void addKnob(Knob* k){ _knobs.push_back(k); }
    
    /*Removes a knob to the vector. This is called by the
     Knob class.*/
    void removeKnob(Knob* k);

};

/******************************FILE_KNOB**************************************/

class File_Knob:public Knob
{
    Q_OBJECT
    
    mutable QMutex _fileSequenceLock;
    std::map<int,QString> _filesSequence;///mapping <frameNumber,fileName>
public:
    
    static Knob* BuildKnob(KnobHolder*  holder, const std::string& description,int dimension){
        return new File_Knob(holder,description,dimension);
    }
    
    File_Knob(KnobHolder* holder, const std::string& description,int dimension):
    Knob(holder,description,dimension)
    {}
    
    virtual void fillHashVector();

    virtual bool canAnimate() const { return false; }
    
    virtual const std::string typeName(){return "InputFile";}
    
    void openFile(){
        emit shouldOpenFile();
    }
    
    /**
     * @brief firstFrame
     * @return Returns the index of the first frame in the sequence held by this Reader.
     */
    int firstFrame() const;
    
    /**
     * @brief lastFrame
     * @return Returns the index of the last frame in the sequence held by this Reader.
     */
	int lastFrame() const;
    
    int frameCount() const{return _filesSequence.size();}
    
    /**
     * @brief nearestFrame
     * @return Returns the index of the nearest frame in the Range [ firstFrame() - lastFrame( ].
     * @param f The index of the frame to modify.
     */
    int nearestFrame(int f) const;
    
    /**
     * @brief getRandomFrameName
     * @param f The index of the frame.
     * @return The file name associated to the frame index. Returns an empty string if it couldn't find it.
     */
    QString getRandomFrameName(int f,bool loadNearestIfNotFound) const;
    
    virtual std::string serialize() const;

    virtual void cloneExtraData(const Knob& other);
    
    virtual void processNewValue();
    
signals:
    void shouldOpenFile();
    
protected:
    
    virtual void _restoreFromString(const std::string& str);
        
};

/******************************OUTPUT_FILE_KNOB**************************************/

class OutputFile_Knob:public Knob
{
    Q_OBJECT
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new OutputFile_Knob(holder,description,dimension);
    }
    
    OutputFile_Knob(KnobHolder* holder, const std::string& description,int dimension):
    Knob(holder,description,dimension)
    {}
    
    virtual void fillHashVector();

    virtual bool canAnimate() const { return false; }
    
    std::string getFileName() const;
    
    virtual const std::string typeName(){return "OutputFile";}
    
    void openFile(){
        emit shouldOpenFile();
    }
    
    virtual std::string serialize() const;

    
signals:
    
    void shouldOpenFile();
    
protected:
    
    virtual void _restoreFromString(const std::string& str);
    
};

/******************************INT_KNOB**************************************/

class Int_Knob:public Knob
{
    
    Q_OBJECT
    
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new Int_Knob(holder,description,dimension);
    }
    
    Int_Knob(KnobHolder* holder, const std::string& description,int dimension):
    Knob(holder,description,dimension)
    ,_disableSlider(false)
    {}

    void disableSlider() { _disableSlider = true;}

    bool isSliderDisabled() const {return _disableSlider;}
    
    virtual void fillHashVector();

    virtual bool canAnimate() const { return true; }
    
    virtual const std::string typeName(){return "Int";}
    
    void setMinimum(int mini,int index = 0){
        if(_minimums.size() > (U32)index){
            _minimums[index] = mini;
        }else{
            if(index == 0){
                _minimums.push_back(mini);
            }else{
                while(_minimums.size() <= (U32)index){
                    _minimums.push_back(INT_MIN);
                }
                _minimums.push_back(mini);
            }
        }
        int maximum = 99;
        if(_maximums.size() > (U32)index){
            maximum = _maximums[index];
        }
        emit minMaxChanged(mini,maximum,index);
    }
    
    void setMaximum(int maxi,int index = 0){
        
        if(_maximums.size() > (U32)index){
            _maximums[index] = maxi;
        }else{
            if(index == 0){
                _maximums.push_back(maxi);
            }else{
                while(_maximums.size() <= (U32)index){
                    _maximums.push_back(INT_MAX);
                }
                _maximums.push_back(maxi);
            }
        }
        int minimum = 99;
        if(_minimums.size() > (U32)index){
            minimum = _minimums[index];
        }
        emit minMaxChanged(minimum,maxi,index);
    }
    
    void setDisplayMinimum(int mini,int index = 0){
        if(_displayMins.size() > (U32)index){
            _displayMins[index] = mini;
        }else{
            if(index == 0){
                _displayMins.push_back(mini);
            }else{
                while(_displayMins.size() <= (U32)index){
                    _displayMins.push_back(0);
                }
                _displayMins.push_back(mini);
            }
        }
    }
    
    void setDisplayMaximum(int maxi,int index = 0){
        
        if(_displayMaxs.size() > (U32)index){
            _displayMaxs[index] = maxi;
        }else{
            if(index == 0){
                _displayMaxs.push_back(maxi);
            }else{
                while(_displayMaxs.size() <= (U32)index){
                    _displayMaxs.push_back(99);
                }
                _displayMaxs.push_back(maxi);
            }
        }
    }
    
    void setIncrement(int incr, int index = 0) {
        assert(incr > 0);
        /*If _increments is already filled, just replace the existing value*/
        if (_increments.size() > (U32)index) {
            _increments[index] = incr;
        }else{
            /*If it is not filled and it is 0, just push_back the value*/
            if(index == 0){
                _increments.push_back(incr);
            } else {
                /*Otherwise fill enough values until we  have
                 the corresponding index in _increments. Then we
                 can push_back the value as the last element of the
                 vector.*/
                while (_increments.size() <= (U32)index) {
                    // FIXME: explain what happens here (comment?)
                    _increments.push_back(1);
                    assert(_increments[_increments.size()-1] > 0);
                }
                _increments.push_back(incr);
            }
        }
        emit incrementChanged(_increments[index],index);
    }
    
    void setIncrement(const std::vector<int>& incr){
        _increments = incr;
        for (U32 i = 0; i < incr.size(); ++i) {
            assert(_increments[i] > 0);
            emit incrementChanged(_increments[i],i);
        }
    }
    
    /*minis & maxis must have the same size*/
    void setMinimumsAndMaximums(const std::vector<int>& minis,const std::vector<int>& maxis){
        _minimums = minis;
        _maximums = maxis;
        for (U32 i = 0; i < maxis.size(); ++i) {
            emit minMaxChanged(_minimums[i], _maximums[i],i);
        }
    }
    
    void setDisplayMinimumsAndMaximums(const std::vector<int>& minis,const std::vector<int>& maxis){
        _displayMins = minis;
        _displayMaxs = maxis;
    }
    
    const std::vector<int>& getMinimums() const {return _minimums;}
    
    const std::vector<int>& getMaximums() const {return _maximums;}
    
    const std::vector<int>& getIncrements() const {return _increments;}
    
    const std::vector<int>& getDisplayMinimums() const {return _displayMins;}
    
    const std::vector<int>& getDisplayMaximums() const {return _displayMaxs;}
    
    virtual std::string serialize() const;

    
protected:
    
    virtual void _restoreFromString(const std::string& str);
    
signals:
    
    void minMaxChanged(int mini,int maxi,int index = 0);
    
    void incrementChanged(int incr,int index = 0);
    
private:

    std::vector<int> _minimums,_maximums,_increments,_displayMins,_displayMaxs;
    bool _disableSlider;
    
};

/******************************BOOL_KNOB**************************************/

class Bool_Knob:public Knob
{
    
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new Bool_Knob(holder,description,dimension);
    }
    
    Bool_Knob(KnobHolder* holder, const std::string& description,int dimension):
    Knob(holder,description,dimension)
    {}
    
    virtual void fillHashVector();

    virtual bool canAnimate() const { return false; }
    
    virtual const std::string typeName(){return "Bool";}
        
    virtual std::string serialize() const;

protected:
    
    
    virtual void _restoreFromString(const std::string& str);
    
};

/******************************DOUBLE_KNOB**************************************/

class Double_Knob:public Knob
{
    Q_OBJECT
    
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new Double_Knob(holder,description,dimension);
    }
    
    Double_Knob(KnobHolder* holder, const std::string& description,int dimension):
    Knob(holder,description,dimension)
    ,_disableSlider(false)
    {}

    void disableSlider() { _disableSlider = true;}

    bool isSliderDisabled() const {return _disableSlider;}
    
    virtual void fillHashVector();

    virtual bool canAnimate() const { return true; }
    
    virtual std::string serialize() const;
    
    virtual const std::string typeName(){return "Double";}
    
    const std::vector<double>& getMinimums() const {return _minimums;}
    
    const std::vector<double>& getMaximums() const {return _maximums;}
    
    const std::vector<double>& getIncrements() const {return _increments;}
    
    const std::vector<int>& getDecimals() const {return _decimals;}
    
    const std::vector<double>& getDisplayMinimums() const {return _displayMins;}
    
    const std::vector<double>& getDisplayMaximums() const {return _displayMaxs;}
    
    void setMinimum(double mini,int index = 0){
        if(_minimums.size() > (U32)index){
            _minimums[index] = mini;
        }else{
            if(index == 0){
                _minimums.push_back(mini);
            }else{
                while(_minimums.size() <= (U32)index){
                    _minimums.push_back(0);
                }
                _minimums.push_back(mini);
            }
        }
        double maximum = 99;
        if(_maximums.size() > (U32)index){
            maximum = _maximums[index];
        }
        emit minMaxChanged(mini,maximum,index);
    }
    
    void setMaximum(double maxi,int index = 0){
        if(_maximums.size() > (U32)index){
            _maximums[index] = maxi;
        }else{
            if(index == 0){
                _maximums.push_back(maxi);
            }else{
                while(_maximums.size() <= (U32)index){
                    _maximums.push_back(99);
                }
                _maximums.push_back(maxi);
            }
        }
        double minimum = 99;
        if(_minimums.size() > (U32)index){
            minimum = _minimums[index];
        }
        emit minMaxChanged(minimum,maxi,index);
    }
    
    void setDisplayMinimum(double mini,int index = 0){
        if(_displayMins.size() > (U32)index){
            _displayMins[index] = mini;
        }else{
            if(index == 0){
                _displayMins.push_back(DBL_MIN);
            }else{
                while(_displayMins.size() <= (U32)index){
                    _displayMins.push_back(0);
                }
                _displayMins.push_back(mini);
            }
        }
    }
    
    void setDisplayMaximum(double maxi,int index = 0){
        
        if(_displayMaxs.size() > (U32)index){
            _displayMaxs[index] = maxi;
        }else{
            if(index == 0){
                _displayMaxs.push_back(maxi);
            }else{
                while(_displayMaxs.size() <= (U32)index){
                    _displayMaxs.push_back(DBL_MAX);
                }
                _displayMaxs.push_back(maxi);
            }
        }
    }
    
    void setIncrement(double incr, int index = 0) {
        assert(incr > 0.);
        if (_increments.size() > (U32)index) {
            _increments[index] = incr;
        }else{
            if(index == 0){
                _increments.push_back(incr);
            }else{
                while (_increments.size() <= (U32)index) {
                    _increments.push_back(0.1);  // FIXME: explain with a comment what happens here? why 0.1?
                }
                _increments.push_back(incr);
            }
        }
        emit incrementChanged(_increments[index],index);
    }
    
    void setDecimals(int decis,int index = 0){
        if(_decimals.size() > (U32)index){
            _decimals[index] = decis;
        }else{
            if(index == 0){
                _decimals.push_back(decis);
            }else{
                while(_decimals.size() <= (U32)index){
                    _decimals.push_back(3);
                    _decimals.push_back(decis);
                }
            }
        }
        emit decimalsChanged(_decimals[index],index);
    }
    
    
    /*minis & maxis must have the same size*/
    void setMinimumsAndMaximums(const std::vector<double>& minis,const std::vector<double>& maxis){
        _minimums = minis;
        _maximums = maxis;
        for (U32 i = 0; i < maxis.size(); ++i) {
            emit minMaxChanged(_minimums[i], _maximums[i],i);
        }
    }
    
    void setDisplayMinimumsAndMaximums(const std::vector<double>& minis,const std::vector<double>& maxis){
        _displayMins = minis;
        _displayMaxs = maxis;
    }
    
    void setIncrement(const std::vector<double>& incr){
        _increments = incr;
        for (U32 i = 0; i < incr.size(); ++i) {
            emit incrementChanged(_increments[i],i);
        }
    }
    void setDecimals(const std::vector<int>& decis){
        _decimals = decis;
        for (U32 i = 0; i < decis.size(); ++i) {
            emit decimalsChanged(decis[i],i);
        }
    }
protected:
    
    virtual void _restoreFromString(const std::string& str);
signals:
    void minMaxChanged(double mini,double maxi,int index = 0);
    
    void incrementChanged(double incr,int index = 0);
    
    void decimalsChanged(int deci,int index = 0);
    
    
private:
    
    std::vector<double> _minimums,_maximums,_increments,_displayMins,_displayMaxs;
    std::vector<int> _decimals;
    bool _disableSlider;
    
};

/******************************BUTTON_KNOB**************************************/

class Button_Knob:public Knob
{
    
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new Button_Knob(holder,description,dimension);
    }
    
    Button_Knob(KnobHolder*  holder, const std::string& description,int dimension):
    Knob(holder,description,dimension){}
    
    virtual void fillHashVector(){}

    virtual bool canAnimate() const { return false; }
    
    virtual const std::string typeName(){return "Button";}
    
    virtual std::string serialize() const{return "";}

protected:
    
    virtual void _restoreFromString(const std::string& str){(void)str;}
};

/******************************COMBOBOX_KNOB**************************************/

class ComboBox_Knob:public Knob
{
    Q_OBJECT
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new ComboBox_Knob(holder,description,dimension);
    }
    
    ComboBox_Knob(KnobHolder* holder, const std::string& description,int dimension):
    Knob(holder,description,dimension)
    {
        
    }

    virtual bool canAnimate() const { return false; }
    
    virtual void fillHashVector();
    
    virtual const std::string typeName(){return "ComboBox";}
    
    /*Must be called right away after the constructor.*/
    void populate(const std::vector<std::string>& entries,const std::vector<std::string>& entriesHelp = std::vector<std::string>()){
        assert(_entriesHelp.empty() || _entriesHelp.size() == entries.size());
        _entriesHelp = entriesHelp;
        _entries = entries;
        emit populated();
    }
    
    const std::vector<std::string>& getEntries() const {return _entries;}
    
    const std::vector<std::string>& getEntriesHelp() const {return _entriesHelp;}
    
    int getActiveEntry() const {return getValue<int>();}
    
    const std::string& getActiveEntryText() const { return _entries[getActiveEntry()]; }
    
    virtual std::string serialize() const;
protected:
    
    virtual void _restoreFromString(const std::string& str);
signals:
    
    void populated();
private:
    std::vector<std::string> _entries;
    std::vector<std::string> _entriesHelp;
};

/******************************SEPARATOR_KNOB**************************************/

class Separator_Knob:public Knob
{
    
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new Separator_Knob(holder,description,dimension);
    }
    
    Separator_Knob(KnobHolder* holder, const std::string& description,int dimension):
    Knob(holder,description,dimension){
        
    }

    virtual bool canAnimate() const { return false; }
    
    virtual void fillHashVector(){}
    
    virtual const std::string typeName(){return "Separator";}
    
    virtual std::string serialize() const{return "";}
protected:
    
    virtual void _restoreFromString(const std::string& str){(void)str;}
};
/******************************RGBA_KNOB**************************************/

/**
 * @brief A color knob with of variable dimension. Each color is a double ranging in [0. , 1.]
 * In dimension 1 the knob will have a single channel being a gray-scale
 * In dimension 3 the knob will have 3 channel R,G,B
 * In dimension 4 the knob will have R,G,B and A channels.
**/
class Color_Knob:public Knob
{
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new Color_Knob(holder,description,dimension);
    }
    

    Color_Knob(KnobHolder* holder, const std::string& description,int dimension):
    Knob(holder,description,dimension)
    {
        //dimension greater than 4 is not supported. Dimension 2 doesn't make sense.
        assert(dimension <= 4 && dimension != 2);
    }
    
    virtual bool canAnimate() const { return true; }

    virtual void fillHashVector();
    
    virtual const std::string typeName(){return "Color";}
    
    virtual std::string serialize() const;

protected:
    
    virtual void _restoreFromString(const std::string& str);
    
};

/******************************STRING_KNOB**************************************/
class String_Knob:public Knob
{
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new String_Knob(holder,description,dimension);
    }
    
    String_Knob(KnobHolder* holder, const std::string& description,int dimension):
    Knob(holder,description,dimension){
        
    }
    
    virtual bool canAnimate() const { return false; }

    virtual void fillHashVector();
    
    virtual const std::string typeName(){return "String";}
    
    std::string getString() const {return getValue<QString>().toStdString();}
    
    virtual std::string serialize() const;
protected:
    
    
    virtual void _restoreFromString(const std::string& str);
private:
    QStringList _entries;
};
/******************************GROUP_KNOB**************************************/
class Group_Knob:public Knob
{
    Q_OBJECT
    
    std::vector<Knob*> _children;
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new Group_Knob(holder,description,dimension);
    }
    
    Group_Knob(KnobHolder* holder, const std::string& description,int dimension):
    Knob(holder,description,dimension)
    {
        
    }

    virtual bool canAnimate() const { return false; }
    
    virtual void fillHashVector(){}
    
    virtual const std::string typeName(){return "Group";}
    
    void addKnob(Knob* k);
    
    const std::vector<Knob*>& getChildren() const {return _children;}
    
    virtual std::string serialize() const{return "";}
protected:
    
    virtual void _restoreFromString(const std::string& str){(void)str;}
};
/******************************TAB_KNOB**************************************/

class Tab_Knob:public Knob
{
    Q_OBJECT
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new Tab_Knob(holder,description,dimension);
    }
    
    Tab_Knob(KnobHolder* holder, const std::string& description,int dimension):
    Knob(holder,description,dimension){
        
    }
    
    virtual bool canAnimate() const { return false; }

    virtual void fillHashVector(){}
    
    virtual const std::string typeName(){return "Tab";}
    
    void addTab(const std::string& typeName);
    
    void addKnob(const std::string& tabName,Knob* k);
    
    const std::map<std::string,std::vector<Knob*> >& getKnobs() const {return _knobs;}
    
    virtual std::string serialize() const{return "";}
    
protected:
    
    virtual void _restoreFromString(const std::string& str){(void)str;}
    
private:
    std::map<std::string,std::vector<Knob*> > _knobs;
};

/******************************RichText_Knob**************************************/
class RichText_Knob:public Knob
{
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new RichText_Knob(holder,description,dimension);
    }
    
    RichText_Knob(KnobHolder* holder, const std::string& description,int dimension):
    Knob(holder,description,dimension){
        
    }
    
    virtual bool canAnimate() const { return false; }

    virtual void fillHashVector();
    
    virtual std::string serialize() const;
    
    virtual const std::string typeName(){return "RichText";}
    
    std::string getString() const {return getValue<QString>().toStdString();}
    
protected:
    
    virtual void _restoreFromString(const std::string& str);

};

#endif // KNOB_H
