// clap_test_scan.cpp
//

#include "clap_test_scan.h"

#include <clap/clap.h>

#include <QTextStream>
#include <QFileInfo>
#include <QDir>

#include <stdint.h>
#include <dlfcn.h>


//----------------------------------------------------------------------
// class clap_test_scan::Impl -- CLAP plugin interface impl.
//

class clap_test_scan::Impl
{
public:

	// Constructor.
	Impl() : m_module(nullptr),
		m_entry(nullptr), m_factory(nullptr), m_plugin(nullptr)
	{
		::memset(&m_host, 0, sizeof(m_host));

		m_host.host_data = this;
		m_host.clap_version = CLAP_VERSION;
		m_host.name = "clap_test_scan";
		m_host.version = "0.0.1";
		m_host.vendor = "rncbc.org";
		m_host.url = "https://github.com/rncbc/clap_test_scan";
		m_host.get_extension = clap_test_scan::Impl::get_extension;
		m_host.request_restart = clap_test_scan::Impl::request_restart;
		m_host.request_process = clap_test_scan::Impl::request_process;
		m_host.request_callback = clap_test_scan::Impl::request_callback;

		clear();
	}

	// destructor.
	~Impl() { close_descriptor(); close(); }

	static const void *get_extension(const clap_host *host, const char *ext_id)
		{ return nullptr; }

	static void request_restart (const clap_host *host) {}
	static void request_process (const clap_host *host) {}
	static void request_callback(const clap_host *host) {}

	// File loader.
	bool open ( const QString& sFilename )
	{
		close();

		const QByteArray aFilename = sFilename.toUtf8();
		m_module = ::dlopen(aFilename.constData(), RTLD_LOCAL | RTLD_LAZY);
		if (!m_module)
			return false;

		m_entry = reinterpret_cast<const clap_plugin_entry *> (
			::dlsym(m_module, "clap_entry"));
		if (!m_entry)
			return false;

		m_entry->init(aFilename.constData());

		m_factory = static_cast<const clap_plugin_factory *> (
			m_entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
		if (!m_factory)
			return false;

		return true;
	}

	bool open_descriptor ( unsigned long iIndex )
	{
		if (!m_factory)
			return false;

		close_descriptor();

		auto count = m_factory->get_plugin_count(m_factory);
		if (iIndex >= count)
			return false;

		auto desc = m_factory->get_plugin_descriptor(m_factory, iIndex);
		if (!desc) {
			qDebug("clap_test_scan::Impl[%p]::open_descriptor(%lu)"
				" *** No plug-in descriptor.", this, iIndex);
			return false;
		}

		if (!clap_version_is_compatible(desc->clap_version)) {
			qDebug("clap_test_scan::Impl[%p]::open_descriptor(%lu)"
				" *** Incompatible CLAP version:"
				" plug-in is %d.%d.%d, host is %d.%d.%d.", this, iIndex,
				desc->clap_version.major,
				desc->clap_version.minor,
				desc->clap_version.revision,
				CLAP_VERSION.major,
				CLAP_VERSION.minor,
				CLAP_VERSION.revision);
			return false;
		}

		m_plugin = m_factory->create_plugin(m_factory, &m_host, desc->id);
		if (!m_plugin) {
			qDebug("clap_test_scan::Impl[%p]::open_descriptor(%lu)"
				" *** Could not create plug-in with id: %s.", this, iIndex, desc->id);
			return false;
		}

		if (!m_plugin->init(m_plugin)) {
			qDebug("clap_test_scan::Impl[%p]::open_descriptor(%lu)"
				" *** Could not initialize plug-in with id: %s.", this, iIndex, desc->id);
			m_plugin->destroy(m_plugin);
			m_plugin = nullptr;
			return false;
		}

		m_sName = desc->name;
		m_iUniqueID = qHash(desc->id);

		const clap_plugin_audio_ports *audio_ports
			= static_cast<const clap_plugin_audio_ports *> (
				m_plugin->get_extension(m_plugin, CLAP_EXT_AUDIO_PORTS));
		if (audio_ports && audio_ports->count && audio_ports->get) {
			clap_audio_port_info info;
			const uint32_t nins = audio_ports->count(m_plugin, true);
			for (uint32_t i = 0; i < nins; ++i) {
				::memset(&info, 0, sizeof(info));
				if (audio_ports->get(m_plugin, i, true, &info)) {
					if (info.flags & CLAP_AUDIO_PORT_IS_MAIN)
						m_iAudioIns += info.channel_count;
				}
			}
			const uint32_t nouts = audio_ports->count(m_plugin, false);
			for (uint32_t i = 0; i < nouts; ++i) {
				::memset(&info, 0, sizeof(info));
				if (audio_ports->get(m_plugin, i, false, &info)) {
					if (info.flags & CLAP_AUDIO_PORT_IS_MAIN)
						m_iAudioOuts += info.channel_count;
				}
			}
		}

		const clap_plugin_note_ports *note_ports
			= static_cast<const clap_plugin_note_ports *> (
				m_plugin->get_extension(m_plugin, CLAP_EXT_NOTE_PORTS));
		if (note_ports && note_ports->count && note_ports->get) {
			clap_note_port_info info;
			const uint32_t nins = note_ports->count(m_plugin, true);
			for (uint32_t i = 0; i < nins; ++i) {
				::memset(&info, 0, sizeof(info));
				if (note_ports->get(m_plugin, i, true, &info)) {
				//	if (info.supported_dialects & CLAP_NOTE_DIALECT_MIDI)
						++m_iMidiIns;
				}
			}
			const uint32_t nouts = note_ports->count(m_plugin, false);
			for (uint32_t i = 0; i < nouts; ++i) {
				::memset(&info, 0, sizeof(info));
				if (note_ports->get(m_plugin, i, false, &info)) {
				//	if (info.supported_dialects & CLAP_NOTE_DIALECT_MIDI)
						++m_iMidiOuts;
				}
			}
		}

		const clap_plugin_params *params
			= static_cast<const clap_plugin_params *> (
				m_plugin->get_extension(m_plugin, CLAP_EXT_PARAMS));
		if (params && params->count && params->get_info) {
			clap_param_info info;
			const uint32_t nparams = params->count(m_plugin);
			for (uint32_t i = 0; i < nparams; ++i) {
				::memset(&info, 0, sizeof(info));
				if (params->get_info(m_plugin, i, &info)) {
					if (info.flags & CLAP_PARAM_IS_READONLY)
						++m_iControlOuts;
					else
						++m_iControlIns;
				}
			}
		}

		const clap_plugin_gui *gui
			= static_cast<const clap_plugin_gui *> (
				m_plugin->get_extension(m_plugin, CLAP_EXT_GUI));
		if (gui && gui->is_api_supported && gui->create && gui->destroy) {
			m_bEditor = (
				gui->is_api_supported(m_plugin, CLAP_WINDOW_API_X11,     false) ||
				gui->is_api_supported(m_plugin, CLAP_WINDOW_API_X11,     true)  ||
				gui->is_api_supported(m_plugin, CLAP_WINDOW_API_WAYLAND, false) ||
				gui->is_api_supported(m_plugin, CLAP_WINDOW_API_WAYLAND, true)
			);
		}

		return true;
	}

	void close_descriptor ()
	{
		if (m_plugin) {
			m_plugin->destroy(m_plugin);
			m_plugin = nullptr;
		}

		clear();
	}

	void close ()
	{
		if (!m_module)
			return;

		m_factory = nullptr;

		if (m_entry) {
			m_entry->deinit();
			m_entry = nullptr;
		}

		::dlclose(m_module);
		m_module = nullptr;
	}

	bool isOpen () const
		{ return (m_module && m_entry && m_factory); }

	// Properties.
	const QString& name() const
		{ return m_sName; }

	unsigned int uniqueID() const
		{ return m_iUniqueID; }

	int controlIns() const
		{ return m_iControlIns; }
	int controlOuts() const
		{ return m_iControlOuts; }

	int audioIns() const
		{ return m_iAudioIns; }
	int audioOuts() const
		{ return m_iAudioOuts; }

	int midiIns() const
		{ return m_iMidiIns; }
	int midiOuts() const
		{ return m_iMidiOuts; }

	bool hasEditor() const
		{ return m_bEditor; }

protected:

	// Cleaner/wiper.
	void clear ()
	{
		m_sName.clear();
		m_iUniqueID  = 0;
		m_iControlIns  = 0;
		m_iControlOuts = 0;
		m_iAudioIns  = 0;
		m_iAudioOuts = 0;
		m_iMidiIns   = 0;
		m_iMidiOuts  = 0;
		m_bEditor = false;
	}

private:

	// Instance variables.
	void *m_module;

	const clap_plugin_entry *m_entry;
	const clap_plugin_factory *m_factory;
	const clap_plugin *m_plugin;

	clap_host m_host;

	QString       m_sName;
	unsigned long m_iIndex;
	unsigned int  m_iUniqueID;
	int           m_iControlIns;
	int           m_iControlOuts;
	int           m_iAudioIns;
	int           m_iAudioOuts;
	int           m_iMidiIns;
	int           m_iMidiOuts;
	bool          m_bEditor;
};


//----------------------------------------------------------------------
// class clap_test_scan -- CLAP plugin interface
//

// Constructor.
clap_test_scan::clap_test_scan (void) : m_pImpl(new Impl())
{
}


// destructor.
clap_test_scan::~clap_test_scan (void)
{
	close_descriptor();
	close();

	delete m_pImpl;
}


// File loader.
bool clap_test_scan::open ( const QString& sFilename )
{
	close();

#ifdef CONFIG_DEBUG_0
	qDebug("clap_test_scan[%p]::open(\"%s\")", this, sFilename.toUtf8().constData());
#endif

	return m_pImpl->open(sFilename);
}


bool clap_test_scan::open_descriptor ( unsigned long iIndex )
{
	close_descriptor();

#ifdef CONFIG_DEBUG_0
	qDebug("clap_test_scan[%p]::open_descriptor( %lu)", this, iIndex);
#endif

	return m_pImpl->open_descriptor(iIndex);
}


// File unloader.
void clap_test_scan::close_descriptor (void)
{
#ifdef CONFIG_DEBUG_0
	qDebug("clap_test_scan[%p]::close_descriptor()", this);
#endif

	m_pImpl->close_descriptor();
}


void clap_test_scan::close (void)
{
#ifdef CONFIG_DEBUG_0
	qDebug("clap_test_scan[%p]::close()", this);
#endif

	m_pImpl->close();
}


// Properties.
bool clap_test_scan::isOpen (void) const
	{ return m_pImpl->isOpen(); }

const QString& clap_test_scan::name (void) const
	{ return m_pImpl->name(); }

unsigned int clap_test_scan::uniqueID (void) const
	{ return m_pImpl->uniqueID(); }

int clap_test_scan::controlIns (void) const
	{ return m_pImpl->controlIns(); }
int clap_test_scan::controlOuts (void) const
	{ return m_pImpl->controlOuts(); }

int clap_test_scan::audioIns (void) const
	{ return m_pImpl->audioIns(); }
int clap_test_scan::audioOuts (void) const
	{ return m_pImpl->audioOuts(); }

int clap_test_scan::midiIns (void) const
	{ return m_pImpl->midiIns(); }
int clap_test_scan::midiOuts (void) const
	{ return m_pImpl->midiOuts(); }

bool clap_test_scan::hasEditor (void) const
	{ return m_pImpl->hasEditor(); }


//-------------------------------------------------------------------------
// clap_test_scan_file - The main scan procedure.
//

static void clap_test_scan_file ( const QString& sFilename )
{
#ifdef CONFIG_DEBUG
	qDebug("clap_test_scan_file(\"%s\")", sFilename.toUtf8().constData());
#endif

	clap_test_scan plugin;

	if (!plugin.open(sFilename))
		return;

	QTextStream sout(stdout);
	unsigned long i = 0;
	while (plugin.open_descriptor(i)) {
		sout << "CLAP|";
		sout << plugin.name() << '|';
		sout << plugin.audioIns()   << ':' << plugin.audioOuts()   << '|';
		sout << plugin.midiIns()    << ':' << plugin.midiOuts()    << '|';
		sout << plugin.controlIns() << ':' << plugin.controlOuts() << '|';
		QStringList flags;
		if (plugin.hasEditor())
			flags.append("GUI");
		flags.append("EXT");
		flags.append("RT");
		sout << flags.join(",") << '|';
		sout << sFilename << '|' << i << '|';
		sout << "0x" << QString::number(plugin.uniqueID(), 16) << '\n';
		plugin.close_descriptor();
		++i;
	}

	plugin.close();

	// Must always give an answer, even if it's a wrong one...
	if (i == 0)
		sout << "clap_test_scan: " << sFilename << ": plugin file error.\n";
}


//-------------------------------------------------------------------------
// main - The main program trunk.
//

#include <QCoreApplication>


int main ( int argc, char **argv )
{
	QCoreApplication app(argc, argv);
#ifdef CONFIG_DEBUG
	qDebug("clap_test_scan: hello.");
#endif

	QTextStream sin(stdin);
	while (!sin.atEnd()) {
		const QString& sLine = sin.readLine();
		if (sLine.isEmpty())
			break;
		clap_test_scan_file(sLine);
	}

#ifdef CONFIG_DEBUG
	qDebug("clap_test_scan: bye.");
#endif
	return 0;
}


// end of clap_test_scan.cpp
