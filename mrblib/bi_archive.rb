
class Bi::Archive
  attr_accessor :path, :index
  attr_accessor :callback,:on_progress
  attr_reader :error

  class Entry
    attr_accessor :path, :encrypted, :start, :size
    def initialize(path,start,size)
      @path = path
      @start = start
      @size = size
      @encrypted = true
    end
  end

  def self.load(path,secret,&callback)
    Bi::Archive.new(path,secret).load(&callback)
  end

  def emscripten?
    self.respond_to? :_download
  end

  #
  def load(&callback)
    if emscripten?
      self._download callback
    else
      _open if File.file?(self.path)
      @available = true
      callback.call(self) if callback
    end
    self
  end

  def available?
    @available
  end

  #
  def filenames
    @index.keys
  end

  #
  def _set_raw_index(raw_index)
    @error = nil
    tmp = nil
    begin
      tmp = JSON::load raw_index
    rescue => e
      @error = "#{e}"
    end
    if @error
      @index = nil
    else
      @index = tmp.to_h{|i| [i.first, Entry.new(*i)] }
    end
  end

  #
  def read(name)
    e = @index[name]
    if e
      self._read e.start,e.size
    elsif File.file?(name)
      File.open(name,"rb"){|f| f.read }
    end
  end

  # load texture image
  def texture(name,straight_alpha=false)
    e = @index[name]
    if e
      t = self._texture e.start,e.size,e.encrypted,straight_alpha
      e.encrypted = false if emscripten?
      t
    elsif File.file?(name)
      Bi::Texture.new name, straight_alpha
    end
  end

end
