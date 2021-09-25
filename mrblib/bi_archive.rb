
class Bi::Archive
  attr_accessor :path, :index
  attr_accessor :callback,:on_progress

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
      _open if File.exists?(self.path)
      @available = true
      callback.call(self) if callback
    end
    return nil
  end

  def available?
    @available
  end

  #
  def filenames
    @index.keys
  end

  #
  def _set_index(index)
    @index = index.to_h{|i| [i.first, Entry.new(*i)] }
  end

  #
  def read(name)
    e = @index[name]
    if e
      self._read e.start,e.size
    elsif File.file?(name)
      File.open(name).read
    end
  end

  # load texture image
  def texture(name,antialias=false)
    e = @index[name]
    if e
      t = self._texture e.start,e.size,e.encrypted,antialias
      e.encrypted = false if emscripten?
      t
    elsif File.file?(name)
      Bi::Texture.new name, antialias
    end
  end

  # load music
  def music(name)
    if @index.include? name
      Bi::Music.new self.read(name)
    elsif File.file?(name)
      Bi::Music.new File.open(name).read()
    end
  end

  # load sound
  def sound(name)
    if @index.include? name
      Bi::Sound.new self.read(name)
    elsif File.file?(name)
      Bi::Sound.new File.open(name).read
    end
  end

end
