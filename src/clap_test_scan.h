// clap_test_scan.h
//

#ifndef __clap_test_scan_h
#define __clap_test_scan_h

#include <QString>


//----------------------------------------------------------------------
// class clap_test_scan -- CLAP plugin (bare bones) interface.
//

class clap_test_scan
{
public:

	// Constructor.
	clap_test_scan();

	// destructor.
	~clap_test_scan();

	// File loader.
	bool open(const QString& sFilename);
	bool open_descriptor(unsigned long iIndex);
	void close_descriptor();
	void close();

	// Properties.
	bool isOpen() const;

	// Properties.
	const QString& name() const;
	unsigned int uniqueID() const;

	int controlIns() const;
	int controlOuts() const;

	int audioIns() const;
	int audioOuts() const;

	int midiIns() const;
	int midiOuts() const;

	bool hasEditor() const;

protected:

	// Forward decls.
	class Impl;

private:

	// Instance variables.
	Impl *m_pImpl;
};


#endif	// __clap_test_scan_h

// end of clap_test_scan.h
